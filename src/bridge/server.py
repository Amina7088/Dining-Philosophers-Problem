import argparse
import json
import mimetypes
import queue
import re
import subprocess
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

HERE = Path(__file__).parent
FRONTEND = (HERE.parent / "frontend").resolve()

# C програмын мөр бүрийг JSON event болгох дүрмүүд (id, fork дугаарыг 0-ээс эхлүүлнэ)
PATTERNS = [
    (re.compile(r"Philosopher (\d+) is THINKING"),
     lambda m: {"type": "state", "id": int(m[1]) - 1, "state": "thinking"}),
    (re.compile(r"Philosopher (\d+) is HUNGRY"),
     lambda m: {"type": "state", "id": int(m[1]) - 1, "state": "hungry"}),
    (re.compile(r"Philosopher (\d+) takes fork (\d+) and (\d+) \(EATING\)"),
     lambda m: {"type": "state", "id": int(m[1]) - 1, "state": "eating",
                "forks": [int(m[2]) - 1, int(m[3]) - 1]}),
    (re.compile(r"Philosopher (\d+) putting fork (\d+) and (\d+) down \(THINKING\)"),
     lambda m: {"type": "state", "id": int(m[1]) - 1, "state": "thinking",
                "released": [int(m[2]) - 1, int(m[3]) - 1]}),
]

clients = []   # SSE-ээр холбогдсон browser бүрийн дараалал
history = []   # өмнө гарсан event-үүд (хожуу холбогдсон хуудаст өгөх)
lock = threading.Lock()
run_id = 0            # Start бүрд нэмэгдэнэ, хуучин ажиллалтыг зогсооно
current_proc = None   # ажиллаж байгаа C процесс
CONFIG = None         # командын мөрийн тохиргоо


def parse_line(line):
    for regex, build in PATTERNS:
        m = regex.search(line)
        if m:
            return build(m)
    return None


def broadcast(event):
    data = json.dumps(event)
    with lock:
        history.append(data)
        del history[:-2000]
        for q in clients:
            q.put(data)


def validate(body):
    """(n, thinkMs, eatMs) эсвэл алдааны мессеж буцаана."""
    try:
        n = int(body["n"])
        think_ms = int(body["thinkMs"])
        eat_ms = int(body["eatMs"])
    except (KeyError, TypeError, ValueError):
        return None, "n, thinkMs, eatMs гэсэн бүхэл тоон талбарууд шаардлагатай."
    if not 2 <= n <= 10:
        return None, "Философичдын тоо 2–10 байх ёстой."
    if not (100 <= think_ms <= 10000 and 100 <= eat_ms <= 10000):
        return None, "Хугацаа 0.1–10 секундын хооронд байх ёстой."
    if (CONFIG.replay or not CONFIG.pass_args) and n != 5:
        return None, "N-ийг өөрчлөхийн тулд серверийг --pass-args горимоор ажиллуулна уу (одоогийн горимд зөвхөн N=5)."
    return (n, think_ms, eat_ms), None


def lines_from(my_id, n, think_ms, eat_ms):
    global current_proc
    if CONFIG.replay:
        for line in Path(CONFIG.replay).read_text(encoding="utf-8").splitlines():
            if my_id != run_id:
                return
            time.sleep(CONFIG.delay)
            yield line
    else:
        cmd = ["stdbuf", "-oL", CONFIG.binary]
        if CONFIG.pass_args:
            cmd += [str(n), str(think_ms), str(eat_ms)]
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, text=True)
        with lock:
            current_proc = proc
        try:
            for line in proc.stdout:
                yield line
        finally:
            proc.terminate()
            proc.wait()


def source_loop(my_id, n, think_ms, eat_ms):
    t0 = time.time()
    for line in lines_from(my_id, n, think_ms, eat_ms):
        if my_id != run_id:
            return
        event = parse_line(line.strip())
        if event is None:
            continue
        event["t"] = int((time.time() - t0) * 1000)
        broadcast(event)
    if my_id == run_id:
        broadcast({"type": "stopped"})


def stop_run(announce=True):
    global run_id, current_proc
    with lock:
        run_id += 1
        proc = current_proc
        current_proc = None
    if proc is not None:
        proc.terminate()
        proc.wait()
    if announce:
        broadcast({"type": "stopped"})


def start_run(n, think_ms, eat_ms):
    global run_id
    stop_run(announce=False)
    with lock:
        run_id += 1
        my_id = run_id
        history.clear()
    broadcast({"type": "reset", "n": n, "thinkMs": think_ms, "eatMs": eat_ms})
    threading.Thread(target=source_loop, args=(my_id, n, think_ms, eat_ms),
                     daemon=True).start()


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def reply(self, code, obj):
        data = json.dumps(obj).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        if self.path == "/events":
            self.sse()
        else:
            self.serve_static()

    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        try:
            body = json.loads(self.rfile.read(length) or b"{}")
        except json.JSONDecodeError:
            self.reply(400, {"ok": False, "error": "JSON буруу байна."})
            return
        if self.path == "/start":
            params, err = validate(body)
            if err:
                self.reply(400, {"ok": False, "error": err})
                return
            start_run(*params)
            self.reply(200, {"ok": True})
        elif self.path == "/stop":
            stop_run()
            self.reply(200, {"ok": True})
        else:
            self.send_error(404)

    def serve_static(self):
        rel = self.path.split("?")[0].lstrip("/") or "index.html"
        target = (FRONTEND / rel).resolve()
        if FRONTEND not in target.parents or not target.is_file():
            self.send_error(404)
            return
        ctype = mimetypes.guess_type(str(target))[0] or "application/octet-stream"
        if ctype.startswith("text/"):
            ctype += "; charset=utf-8"
        body = target.read_bytes()
        self.send_response(200)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def sse(self):
        self.send_response(200)
        self.send_header("Content-Type", "text/event-stream")
        self.send_header("Cache-Control", "no-cache")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        q = queue.Queue()
        with lock:
            for old in history:
                q.put(old)
            clients.append(q)
        try:
            while True:
                try:
                    self.wfile.write(f"data: {q.get(timeout=15)}\n\n".encode())
                except queue.Empty:
                    self.wfile.write(b": keepalive\n\n")
                self.wfile.flush()
        except (BrokenPipeError, ConnectionResetError):
            pass
        finally:
            with lock:
                clients.remove(q)


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--replay", help="C програмын гаралтын файл, жишээ нь examples/sample_run.txt")
    ap.add_argument("--delay", type=float, default=0.3, help="replay үед мөр хоорондын хугацаа (сек)")
    ap.add_argument("--binary", default="./philosophers", help="бодит C програм")
    ap.add_argument("--pass-args", action="store_true",
                    help="C програмд N, thinkMs, eatMs-ийг C програмд аргументаар дамжуулна")
    ap.add_argument("--port", type=int, default=8000)
    CONFIG = ap.parse_args()

    print(f"Server: http://localhost:{CONFIG.port}  (Ctrl+C зогсоох)")
    ThreadingHTTPServer(("", CONFIG.port), Handler).serve_forever()
import argparse
import json
import queue
import re
import subprocess
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

HERE = Path(__file__).parent

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


def lines_from(args):
    if args.replay:
        for line in Path(args.replay).read_text(encoding="utf-8").splitlines():
            time.sleep(args.delay)
            yield line
    else:
        proc = subprocess.Popen(["stdbuf", "-oL", args.binary],
                                stdout=subprocess.PIPE, text=True)
        try:
            for line in proc.stdout:
                yield line
        finally:
            proc.terminate()


def source_loop(args):
    t0 = time.time()
    for line in lines_from(args):
        event = parse_line(line.strip())
        if event is None:
            continue
        event["t"] = int((time.time() - t0) * 1000)
        broadcast(event)
    broadcast({"type": "stopped"})


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def do_GET(self):
        if self.path == "/events":
            self.sse()
        elif self.path in ("/", "/debug.html"):
            page = HERE / "debug.html"
            if not page.exists():
                self.send_error(404, "debug.html not found")
                return
            body = page.read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(body)
        else:
            self.send_error(404)

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
    ap.add_argument("--port", type=int, default=8000)
    args = ap.parse_args()

    threading.Thread(target=source_loop, args=(args,), daemon=True).start()
    print(f"Server: http://localhost:{args.port}  (Ctrl+C зогсоох)")
    ThreadingHTTPServer(("", args.port), Handler).serve_forever()
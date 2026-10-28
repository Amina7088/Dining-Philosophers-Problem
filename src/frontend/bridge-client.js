// Backend-тэй холбох давхарга.
// script.js-ийг өөрчлөхгүй, зөвхөн түүний глобал функц, хувьсагчийг ашиглана.
(function () {
  "use strict";

  let es = null;
  let hasError = false;   // шалгалтын алдаа дэлгэц дээр байгаа эсэх

  // Локал симуляцийн listener-үүдийг авч, backend-ийнхээр сольно.
  startButton.removeEventListener("click", start);
  pauseButton.removeEventListener("click", togglePause);
  resetButton.removeEventListener("click", reset);
  countInput.removeEventListener("change", reset);
  thinkInput.removeEventListener("change", reset);
  eatInput.removeEventListener("change", reset);

  pauseButton.textContent = "■ Stop";   

  async function post(path, body) {
    const res = await fetch(path, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(body || {})
    });
    return res.json();
  }

  function showError(message) {
    hasError = true;
    setBadge("ERROR");
    statusElement.textContent = message;
    alert(message);
  }

  async function onStart() {
    let s;
    try {
      s = getSettings();                // script.js-ийн шалгалт
    } catch (err) {
      showError(err.message);
      return;
    }
    hasError = false;
    try {
      const r = await post("/start", {
        n: s.count,
        thinkMs: Math.round(s.thinkTime * 1000),
        eatMs: Math.round(s.eatTime * 1000)
      });
      if (!r.ok) {
        showError(r.error);
        return;
      }
      startButton.disabled = true;
      pauseButton.disabled = false;
    } catch (err) {
      showError("Серверт хүрч чадсангүй. server.py ажиллаж байгаа эсэхийг шалгана уу.");
    }
  }

  async function onStop() {
    try { await post("/stop"); } catch (err) { /* сервер унтарсан бол үл тооно */ }
  }

  async function onReset() {
    hasError = false;
    await onStop();
    reset();                            // script.js-ийн reset: философичдыг эхний төлөвт буцаана
    pauseButton.textContent = "■ Stop";
    try {
      getSettings();
    } catch (err) {
      showError(err.message);           // буруу утга оруулбал алдааг харуулна
    }
  }

  function handle(ev) {
    if (ev.type === "reset") {
      createPhilosophers(ev.n);
      render();
      setBadge("RUNNING", "running");
      statusElement.textContent = `Backend ажиллаж байна (N=${ev.n}).`;
    } else if (ev.type === "state") {
      const p = philosophers[ev.id];
      if (!p) return;
      p.state = ev.state;
      render();
    } else if (ev.type === "stopped") {
      startButton.disabled = false;
      pauseButton.disabled = true;
      if (hasError) return;             // ERROR мессежийг дарж бичихгүй
      setBadge("READY");
      statusElement.textContent = "Backend зогссон.";
    }
  }

  function connect() {
    es = new EventSource("/events");
    es.onmessage = (e) => handle(JSON.parse(e.data));
    es.onerror = () => showError("Серверийн холболт тасарсан. Дахин холбогдохыг оролдож байна...");
  }

  startButton.addEventListener("click", onStart);
  pauseButton.addEventListener("click", onStop);
  resetButton.addEventListener("click", onReset);
  countInput.addEventListener("change", onReset);
  thinkInput.addEventListener("change", onReset);
  eatInput.addEventListener("change", onReset);

  connect();
})();
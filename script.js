"use strict";

const countInput = document.getElementById("count");
const thinkInput = document.getElementById("thinkTime");
const eatInput = document.getElementById("eatTime");

const philosophersElement = document.getElementById("philosophers");
const statusElement = document.getElementById("status");

const startButton = document.getElementById("start");
const pauseButton = document.getElementById("pause");
const resetButton = document.getElementById("reset");

const thinkingCountElement = document.getElementById("thinkingCount");
const hungryCountElement = document.getElementById("hungryCount");
const eatingCountElement = document.getElementById("eatingCount");
const runBadge = document.getElementById("runBadge");

let philosophers = [];
let running = false;
let paused = false;
let timerId = null;
let lastTick = 0;

const MIN_COUNT = 2;
const MAX_COUNT = 10;
const MIN_TIME = 1;
const MAX_TIME = 10;

function getSettings() {
  const count = Number(countInput.value);
  const thinkTime = Number(thinkInput.value);
  const eatTime = Number(eatInput.value);

  if (!Number.isInteger(count) || count < MIN_COUNT || count > MAX_COUNT) {
    throw new Error("Философичдын тоо 2–10 бүхэл тоо байх ёстой.");
  }

  if (!Number.isFinite(thinkTime) || thinkTime < MIN_TIME || thinkTime > MAX_TIME) {
    throw new Error("Бодох хугацаа 1–10 секундын хооронд байх ёстой.");
  }

  if (!Number.isFinite(eatTime) || eatTime < MIN_TIME || eatTime > MAX_TIME) {
    throw new Error("Хооллох хугацаа 1–10 секундын хооронд байх ёстой.");
  }

  return { count, thinkTime, eatTime };
}

function stopTimer() {
  if (timerId !== null) {
    clearInterval(timerId);
    timerId = null;
  }
}

function setBadge(text, className = "") {
  runBadge.textContent = text;
  runBadge.className = `run-badge ${className}`.trim();
}

function createPhilosophers(count) {
  philosophers = Array.from({ length: count }, () => ({
    state: "thinking",
    elapsed: 0
  }));
}

function canEat(index) {
  const count = philosophers.length;
  const left = (index + count - 1) % count;
  const right = (index + 1) % count;

  return (
    philosophers[left].state !== "eating" &&
    philosophers[right].state !== "eating"
  );
}

function updateSummary() {
  const counts = {
    thinking: 0,
    hungry: 0,
    eating: 0
  };

  for (const philosopher of philosophers) {
    counts[philosopher.state]++;
  }

  thinkingCountElement.textContent = counts.thinking;
  hungryCountElement.textContent = counts.hungry;
  eatingCountElement.textContent = counts.eating;
}

function render() {
  philosophersElement.replaceChildren();

  const count = philosophers.length;

  if (count === 0) {
    updateSummary();
    return;
  }

  const philosopherRadius = count <= 5 ? 37 : 40;
  const forkRadius = count <= 5 ? 23 : 27;

  // Сэрээ бүрийг хөрш хоёр философичийн дунд байрлуулна.
  for (let i = 0; i < count; i++) {
    const angle = ((i + 0.5) / count) * 2 * Math.PI - Math.PI / 2;
    const x = 50 + forkRadius * Math.cos(angle);
    const y = 50 + forkRadius * Math.sin(angle);
    const next = (i + 1) % count;

    const fork = document.createElement("div");
    fork.className = "fork";
    fork.style.setProperty("--x", `${x}%`);
    fork.style.setProperty("--y", `${y}%`);
    fork.style.setProperty(
      "--angle",
      `${(angle * 180) / Math.PI + 90}deg`
    );
    fork.textContent = "♆";

    const occupied =
      philosophers[i].state === "eating" ||
      philosophers[next].state === "eating";

    if (occupied) {
      fork.classList.add("occupied");
      fork.title = `Сэрээ ${i + 1} — ашиглагдаж байна`;
    } else {
      fork.title = `Сэрээ ${i + 1} — сул`;
    }

    philosophersElement.appendChild(fork);
  }

  // Философичдын картуудыг дугуй ширээг тойруулна.
  philosophers.forEach((philosopher, index) => {
    const angle = (index / count) * 2 * Math.PI - Math.PI / 2;
    const x = 50 + philosopherRadius * Math.cos(angle);
    const y = 50 + philosopherRadius * Math.sin(angle);

    const card = document.createElement("div");
    card.className = `philosopher ${philosopher.state}`;
    card.style.setProperty("--x", `${x}%`);
    card.style.setProperty("--y", `${y}%`);

    const name = document.createElement("div");
    name.textContent = `Philosopher ${index + 1}`;

    const state = document.createElement("p");
    state.textContent = philosopher.state.toUpperCase();

    card.append(name, state);
    philosophersElement.appendChild(card);
  });

  updateSummary();
}

function tick() {
  if (!running || paused) return;

  let settings;

  try {
    settings = getSettings();
  } catch (error) {
    stopTimer();
    running = false;
    paused = false;
    startButton.disabled = false;
    pauseButton.disabled = true;
    setBadge("ERROR");
    statusElement.textContent = error.message;
    return;
  }

  const now = Date.now();
  const delta = Math.min((now - lastTick) / 1000, 0.25);
  lastTick = now;

  // 1. Хооллож дууссан философич бодож эхэлнэ.
  for (const philosopher of philosophers) {
    if (
      philosopher.state === "eating" &&
      philosopher.elapsed + delta >= settings.eatTime
    ) {
      philosopher.state = "thinking";
      philosopher.elapsed = 0;
    }
  }

  // 2. Бодох болон хооллох хугацааг шинэчилнэ.
  for (const philosopher of philosophers) {
    if (philosopher.state === "eating") {
      philosopher.elapsed += delta;
    } else if (philosopher.state === "thinking") {
      philosopher.elapsed += delta;

      if (philosopher.elapsed >= settings.thinkTime) {
        philosopher.state = "hungry";
        philosopher.elapsed = 0;
      }
    }
  }

  // 3. Хоёр сэрээ нь сул философичид хооллох боломж олгоно.
  // Бүх шилжилтийг нэг tick дотор дарааллаар шийднэ.
  for (let i = 0; i < philosophers.length; i++) {
    if (philosophers[i].state === "hungry" && canEat(i)) {
      philosophers[i].state = "eating";
      philosophers[i].elapsed = 0;
    }
  }

  render();

  const eatingCount = philosophers.filter(
    (philosopher) => philosopher.state === "eating"
  ).length;

  statusElement.textContent =
    `Симуляци ажиллаж байна. Хооллож буй философич: ${eatingCount}/${philosophers.length}.`;
}

function reset() {
  stopTimer();

  running = false;
  paused = false;

  try {
    const { count } = getSettings();

    createPhilosophers(count);
    render();

    startButton.disabled = false;
    pauseButton.disabled = true;
    pauseButton.textContent = "Ⅱ Pause";

    setBadge("READY");
    statusElement.textContent = "Симуляци эхлүүлэхэд бэлэн.";
  } catch (error) {
    philosophers = [];
    render();

    startButton.disabled = false;
    pauseButton.disabled = true;

    setBadge("ERROR");
    statusElement.textContent = error.message;
  }
}

function start() {
  let settings;

  try {
    settings = getSettings();
  } catch (error) {
    statusElement.textContent = error.message;
    setBadge("ERROR");
    return;
  }

  stopTimer();
  createPhilosophers(settings.count);

  running = true;
  paused = false;
  lastTick = Date.now();

  startButton.disabled = true;
  pauseButton.disabled = false;
  pauseButton.textContent = "Ⅱ Pause";

  setBadge("RUNNING", "running");
  statusElement.textContent =
    "Симуляци эхэллээ. Философичдын төлөвүүд өөрчлөгдөж байна.";

  render();
  timerId = setInterval(tick, 100);
}

function togglePause() {
  if (!running) return;

  paused = !paused;
  lastTick = Date.now();

  if (paused) {
    pauseButton.textContent = "▶ Resume";
    setBadge("PAUSED", "paused");
    statusElement.textContent = "Симуляци түр зогссон.";
  } else {
    pauseButton.textContent = "Ⅱ Pause";
    setBadge("RUNNING", "running");
    statusElement.textContent = "Симуляци үргэлжилж байна.";
  }
}

startButton.addEventListener("click", start);
pauseButton.addEventListener("click", togglePause);
resetButton.addEventListener("click", reset);

countInput.addEventListener("change", reset);
thinkInput.addEventListener("change", reset);
eatInput.addEventListener("change", reset);

// Хуудас нээгдэхэд эхний төлөвийг зурна.
reset();
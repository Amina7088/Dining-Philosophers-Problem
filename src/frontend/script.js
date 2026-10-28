"use strict";

// ========================================
// DOM ELEMENTS
// ========================================

const countInput = document.getElementById("count");
const thinkInput = document.getElementById("thinkTime");
const eatInput = document.getElementById("eatTime");

const philosophersElement = document.getElementById("philosophers");
const philosopherDetailsElement =
  document.getElementById("philosopherDetails");

const statusElement = document.getElementById("status");

const startButton = document.getElementById("start");
const pauseButton = document.getElementById("pause");
const resetButton = document.getElementById("reset");

const thinkingCountElement = document.getElementById("thinkingCount");
const hungryCountElement = document.getElementById("hungryCount");
const eatingCountElement = document.getElementById("eatingCount");

const runBadge = document.getElementById("runBadge");
const philosopherTotalElement = document.getElementById("philosopherTotal");

// ========================================
// STATE
// ========================================

let philosophers = [];

let running = false;
let paused = false;

let timerId = null;
let lastTick = 0;

const MIN_COUNT = 2;
const MAX_COUNT = 10;

const MIN_TIME = 1;
const MAX_TIME = 10;

// ========================================
// SETTINGS
// ========================================

function getSettings() {
  const count = Number(countInput.value);
  const thinkTime = Number(thinkInput.value);
  const eatTime = Number(eatInput.value);

  if (
    !Number.isInteger(count) ||
    count < MIN_COUNT ||
    count > MAX_COUNT
  ) {
    throw new Error(
      "Философичдын тоо 2–10 бүхэл тоо байх ёстой."
    );
  }

  if (
    !Number.isFinite(thinkTime) ||
    thinkTime < MIN_TIME ||
    thinkTime > MAX_TIME
  ) {
    throw new Error(
      "Бодох хугацаа 1–10 секундын хооронд байх ёстой."
    );
  }

  if (
    !Number.isFinite(eatTime) ||
    eatTime < MIN_TIME ||
    eatTime > MAX_TIME
  ) {
    throw new Error(
      "Хооллох хугацаа 1–10 секундын хооронд байх ёстой."
    );
  }

  return {
    count,
    thinkTime,
    eatTime
  };
}

// ========================================
// TIMER
// ========================================

function stopTimer() {
  if (timerId !== null) {
    clearInterval(timerId);
    timerId = null;
  }
}

// ========================================
// BADGE
// ========================================

function setBadge(text, className = "") {
  runBadge.textContent = text;
  runBadge.className = `run-badge ${className}`.trim();
}

// ========================================
// CREATE PHILOSOPHERS
// ========================================

function createPhilosophers(count) {
  philosophers = Array.from(
    { length: count },
    (_, index) => ({
      id: index,
      state: "thinking",
      elapsed: 0,
      meals: 0,
      totalEatingTime: 0,
      waitingTime: 0
    })
  );
}

// ========================================
// NEIGHBOR CHECK
// ========================================

function canEat(index) {
  const count = philosophers.length;

  const left = (index + count - 1) % count;
  const right = (index + 1) % count;

  return (
    philosophers[left].state !== "eating" &&
    philosophers[right].state !== "eating"
  );
}

// ========================================
// SUMMARY
// ========================================

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

  philosopherTotalElement.textContent =
    String(philosophers.length).padStart(2, "0");
}

// ========================================
// PHILOSOPHER DETAILS
// ========================================

function renderPhilosopherDetails() {
  philosopherDetailsElement.replaceChildren();

  philosophers.forEach((philosopher, index) => {
    const row = document.createElement("div");

    row.className = `philosopher-row ${philosopher.state}`;

    const left = document.createElement("div");
    left.className = "philosopher-row-left";

    const number = document.createElement("div");
    number.className = "philosopher-number";
    number.textContent = `P${index + 1}`;

    const name = document.createElement("div");
    name.className = "philosopher-name";
    name.textContent = `Philosopher ${index + 1}`;

    const state = document.createElement("span");
    state.className = "philosopher-state";
    state.textContent = philosopher.state.toUpperCase();

    left.append(number, name);
    row.append(left, state);

    philosopherDetailsElement.appendChild(row);
  });
}

// ========================================
// RENDER TABLE
// ========================================

function render() {
  // Зөвхөн философич болон сэрээний элементүүдийг устгана.
  // .table-center зэрэг HTML-ийн үндсэн элементүүдийг хадгална.

  philosophersElement
    .querySelectorAll(".fork, .philosopher")
    .forEach(element => element.remove());

  const count = philosophers.length;

  if (count === 0) {
    updateSummary();
    renderPhilosopherDetails();
    return;
  }

  // --------------------------------------
  // PHILOSOPHER POSITION
  // --------------------------------------

  const philosopherRadius = count <= 5 ? 37 : 40;

  // --------------------------------------
  // FORK POSITION
  // --------------------------------------

  const forkRadius = count <= 5 ? 24 : 27;

  for (let i = 0; i < count; i++) {
    const angle =
      ((i + 0.5) / count) * 2 * Math.PI -
      Math.PI / 2;

    const x =
      50 + forkRadius * Math.cos(angle);

    const y =
      50 + forkRadius * Math.sin(angle);

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

      fork.title =
        `Сэрээ ${i + 1} — ашиглагдаж байна`;
    } else {
      fork.title =
        `Сэрээ ${i + 1} — сул`;
    }

    philosophersElement.appendChild(fork);
  }

  // --------------------------------------
  // PHILOSOPHER CARDS
  // --------------------------------------

  philosophers.forEach((philosopher, index) => {
    const angle =
      (index / count) * 2 * Math.PI -
      Math.PI / 2;

    const x =
      50 + philosopherRadius * Math.cos(angle);

    const y =
      50 + philosopherRadius * Math.sin(angle);

    const card = document.createElement("div");

    card.className =
      `philosopher ${philosopher.state}`;

    card.style.setProperty("--x", `${x}%`);
    card.style.setProperty("--y", `${y}%`);

    const name = document.createElement("div");
    name.textContent = `P${index + 1}`;

    const state = document.createElement("p");
    state.textContent = philosopher.state.toUpperCase();

    card.append(name, state);

    card.title =
      `Philosopher ${index + 1}: ${philosopher.state}`;

    philosophersElement.appendChild(card);
  });

  updateSummary();
  renderPhilosopherDetails();
}

// ========================================
// SIMULATION TICK
// ========================================

function tick() {
  if (!running || paused) {
    return;
  }

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

  const delta = Math.min(
    (now - lastTick) / 1000,
    0.25
  );

  lastTick = now;

  // --------------------------------------
  // 1. UPDATE WAITING TIME
  // --------------------------------------

  for (const philosopher of philosophers) {
    if (philosopher.state === "hungry") {
      philosopher.waitingTime += delta;
    }
  }

  // --------------------------------------
  // 2. FINISH EATING
  // --------------------------------------

  for (const philosopher of philosophers) {
    if (
      philosopher.state === "eating" &&
      philosopher.elapsed + delta >= settings.eatTime
    ) {
      philosopher.state = "thinking";
      philosopher.elapsed = 0;
    }
  }

  // --------------------------------------
  // 3. UPDATE THINKING / EATING
  // --------------------------------------

  for (const philosopher of philosophers) {
    if (philosopher.state === "eating") {
      philosopher.elapsed += delta;
      philosopher.totalEatingTime += delta;
    } else if (philosopher.state === "thinking") {
      philosopher.elapsed += delta;

      if (philosopher.elapsed >= settings.thinkTime) {
        philosopher.state = "hungry";

        philosopher.elapsed = 0;
        philosopher.waitingTime = 0;
      }
    }
  }

  // --------------------------------------
  // 4. HUNGRY PHILOSOPHERS TRY TO EAT
  // --------------------------------------

  for (let i = 0; i < philosophers.length; i++) {
    const philosopher = philosophers[i];

    if (
      philosopher.state === "hungry" &&
      canEat(i)
    ) {
      philosopher.state = "eating";

      philosopher.elapsed = 0;
      philosopher.meals++;
    }
  }

  // --------------------------------------
  // 5. UPDATE UI
  // --------------------------------------

  render();

  const eatingCount = philosophers.filter(
    philosopher =>
      philosopher.state === "eating"
  ).length;

  const hungryCount = philosophers.filter(
    philosopher =>
      philosopher.state === "hungry"
  ).length;

  statusElement.textContent =
    `Симуляци ажиллаж байна. ` +
    `Хооллож буй: ${eatingCount}/${philosophers.length}. ` +
    `Хүлээж буй: ${hungryCount}.`;
}

// ========================================
// RESET
// ========================================

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

    statusElement.textContent =
      "Симуляци эхлүүлэхэд бэлэн.";

  } catch (error) {
    philosophers = [];

    render();

    startButton.disabled = false;
    pauseButton.disabled = true;

    setBadge("ERROR");

    statusElement.textContent =
      error.message;
  }
}

// ========================================
// START
// ========================================

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

// ========================================
// PAUSE / RESUME
// ========================================

function togglePause() {
  if (!running) {
    return;
  }

  paused = !paused;

  lastTick = Date.now();

  if (paused) {
    pauseButton.textContent = "▶ Resume";

    setBadge("PAUSED", "paused");

    statusElement.textContent =
      "Симуляци түр зогссон.";

  } else {
    pauseButton.textContent = "Ⅱ Pause";

    setBadge("RUNNING", "running");

    statusElement.textContent =
      "Симуляци үргэлжилж байна.";
  }
}

// ========================================
// EVENT LISTENERS
// ========================================

// bridge-client.js холбогдсон үед локал listener-үүдийг
// давхар холбохгүй. Bridge байхгүй үед frontend өөрөө ажиллана.

const bridgeClientLoaded = Array.from(document.scripts).some(
  script => script.src.includes("bridge-client.js")
);

if (!bridgeClientLoaded) {
  startButton.addEventListener("click", start);

  pauseButton.addEventListener("click", togglePause);

  resetButton.addEventListener("click", reset);

  countInput.addEventListener("change", reset);

  thinkInput.addEventListener("change", reset);

  eatInput.addEventListener("change", reset);
}

// ========================================
// INITIAL RENDER
// ========================================

reset();
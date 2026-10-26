
const countInput = document.getElementById("count");
const thinkInput = document.getElementById("thinkTime");
const eatInput = document.getElementById("eatTime");
const philosophersElement = document.getElementById("philosophers");
const statusElement = document.getElementById("status");

const startButton = document.getElementById("start");
const pauseButton = document.getElementById("pause");
const resetButton = document.getElementById("reset");

let philosophers = [];
let running = false;
let paused = false;

function render() {
  philosophersElement.replaceChildren();

  philosophers.forEach((state, index) => {
    const card = document.createElement("div");
    card.className = `philosopher ${state}`;

    const name = document.createElement("div");
    name.textContent = `Philosopher ${index + 1}`;

    const currentState = document.createElement("p");
    currentState.textContent = state.toUpperCase();

    card.append(name, currentState);
    philosophersElement.appendChild(card);
  });
}

function reset() {
  running = false;
  paused = false;

  const count = Number(countInput.value);

  if (!Number.isInteger(count) || count < 2 || count > 10) {
    statusElement.textContent =
      "Алдаа: философичдын тоо 2–10 бүхэл тоо байх ёстой.";
    return;
  }

  philosophers = Array(count).fill("thinking");
  render();

  startButton.disabled = false;
  pauseButton.disabled = true;
  pauseButton.textContent = "Pause";

  statusElement.textContent = "Симуляци эхлүүлэхэд бэлэн.";
}

function start() {
  const thinkTime = Number(thinkInput.value);
  const eatTime = Number(eatInput.value);

  if (
    !Number.isFinite(thinkTime) ||
    !Number.isFinite(eatTime) ||
    thinkTime < 1 || thinkTime > 10 ||
    eatTime < 1 || eatTime > 10
  ) {
    statusElement.textContent =
      "Алдаа: хугацаа 1–10 секундын хооронд байх ёстой.";
    return;
  }

  reset();

  if (philosophers.length === 0) return;

  running = true;
  startButton.disabled = true;
  pauseButton.disabled = false;

  statusElement.textContent =
    "UI бэлэн. Backend холболт хараахан хийгдээгүй.";
}

function togglePause() {
  if (!running) return;

  paused = !paused;
  pauseButton.textContent = paused ? "Resume" : "Pause";

  statusElement.textContent = paused
    ? "Симуляци түр зогссон."
    : "Симуляци үргэлжлэхэд бэлэн.";
}

startButton.addEventListener("click", start);
pauseButton.addEventListener("click", togglePause);
resetButton.addEventListener("click", reset);
countInput.addEventListener("change", reset);

reset();

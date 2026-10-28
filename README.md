# Dining Philosophers Simulator

**Хичээл:** Үйлдлийн систем (F.CSM302) — Бие даалт 1  
**Код / Хуваарилалт:** 4-2-11

---

## Төслийн зорилго

Энэхүү төсөл нь Абрахам Сильбершацын *Operating System Concepts* сурах бичгийн 7-р бүлгийн Project 3 (*The Dining-Philosophers Problem*)-ийн дагуу олон thread-тэй орчин дахь синхрончлол, дундын нөөцийн өрсөлдөөн болон систем гацахаас (*Deadlock*) сэргийлэх механизмыг харуулах зорилготой.

POSIX Threads (`pthreads`), mutex locks болон condition variables ашиглан Monitor шийдлийг C хэл дээр хэрэгжүүлж, Python HTTP/SSE bridge серверээр дамжуулан вэб браузер дээр бодит цаг хугацаанд (*real-time*) визуал байдлаар харуулдаг.

---

## Файлын бүтэц

```text
Dining-Philosophers-Problem/
├── philosophers              # C бинар файл (хөрвүүлсний дараа үүснэ)
├── src/                      # Эх код
│   ├── philosophers.c        # POSIX thread/mutex/cond var бүхий C core код
│   ├── bridge/
│   │   └── server.py         # Python HTTP & SSE Bridge сервер
│   └── frontend/
│       ├── index.html        # Вэб хэрэглэгчийн интерфейс
│       ├── script.js         # Frontend UI логик ба рендер
│       ├── bridge-client.js  # Server-Sent Events (SSE) клиент
│       └── style.css         # UI CSS загвар
├── tests/                    # Автомат тестийн кодууд
│   ├── test_philosophers.c   # C хэл дээрх нэгж болон стресс тестүүд
│   └── results.txt           # Тестийн нэгдсэн үр дүн
├── examples/                 # Дахин тоглуулах (replay) жишээ өгөгдөл
│   └── sample_run.txt        # Төлөвийн лог файл
├── docs/                     # Баримт бичиг ба гарын авлага
│   ├── Manual.pdf            # Системийг ажиллуулах, суулгах гарын авлага
│   └── Report.pdf            # Бие даалтын ажлын нэгдсэн тайлан
└── README.md                 # Төслийн танилцуулга файл
```

---

## Quick Start (Хурдан ажиллуулах заавар)

### Шаардлагатай орчин

- Linux орчин (Debian, Ubuntu эсвэл Windows WSL2)
- GCC хөрвүүлэгч (`pthread` дэмжлэгтэй)
- Python 3.8+

---

### 1. Репозиторийг клон хийх

```bash
git clone https://github.com/Amina7088/Dining-Philosophers-Problem.git
cd Dining-Philosophers-Problem
```

### 2. C программыг хөрвүүлэх

C программыг үндсэн директорт `philosophers` нэртэй executable файл болгон хөрвүүлнэ.

```bash
gcc -O2 src/philosophers.c -o philosophers -pthread
```

### 3. Python Bridge серверийг асаах

Вэб UI-аас философичдын тоо (`N`) болон симуляцийн хугацааны тохиргоог дамжуулах боломжтойгоор серверийг ажиллуулна.

```bash
python3 src/bridge/server.py --pass-args
```

### 4. Симуляторыг нээх

Браузераа нээгээд дараах хаягаар орно.

[http://localhost:8000](http://localhost:8000)

**Start** товчийг дарж симуляцийг эхлүүлнэ.

---

## Тест ажиллуулах

C хэлний алгоритм, синхрончлол болон стресс тестийг ажиллуулахын тулд дараах командуудыг ажиллуулна.

### 1. Тестийн программыг хөрвүүлэх

```bash
gcc tests/test_philosophers.c -o test_ph -pthread
```

### 2. Тест ажиллуулах

```bash
./test_ph
```

**Тестийн үр дүн:** Автомат тестийн 92 шалгалт бүгд амжилттай давсан (`ALL TESTS PASSED`).

---

## Ашигласан сан ба AI хэрэгслийн мэдээлэл

### Ашигласан сангууд

Төсөл нь гадны framework болон нэмэлт сангаас хамааралгүй (*Zero-dependency*) байдлаар хэрэгжсэн.

- **C:** POSIX Threads (`pthread.h`), C Standard Library (`stdio.h`, `stdlib.h`, `unistd.h`)
- **Python:** Стандарт сангууд (`http.server`, `subprocess`, `argparse`, `re`)
- **Frontend:** HTML5, CSS3, JavaScript (ES6 `EventSource` API)

### AI хэрэгслийн ашиглалт

Кодын зарим функцийг оновчлох, архитектурын зөвлөгөө авах, тестийн тохиолдлууд боловсруулах болон баримт бичгийн найруулгыг сайжруулахад **Gemini** болон **Claude** AI туслах хэрэгслүүдийг ашигласан болно.

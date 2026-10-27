#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_N 10
#define THINKING 0
#define HUNGRY 1
#define EATING 2

#define LEFT (phnum + N - 1) % N
#define RIGHT (phnum + 1) % N

int N = 5;
int state[MAX_N];
int phil[MAX_N];

int think_ms = 1000;    // бодох хугацаа (мс)
int eat_ms = 2000;      // хооллох хугацаа (мс)

pthread_mutex_t mutex;
pthread_cond_t cond[MAX_N];

void test(int phnum) {
    if (state[phnum] == HUNGRY &&
        state[LEFT] != EATING &&
        state[RIGHT] != EATING) {

        state[phnum] = EATING;
        printf("Philosopher %d takes fork %d and %d (EATING)\n", phnum + 1, LEFT + 1, phnum + 1);

        pthread_cond_signal(&cond[phnum]);
    }
}

void pickup_forks(int phnum) {
    pthread_mutex_lock(&mutex);

    state[phnum] = HUNGRY;
    printf("Philosopher %d is HUNGRY\n", phnum + 1);

    test(phnum);

    while (state[phnum] != EATING) {
        pthread_cond_wait(&cond[phnum], &mutex);
    }

    pthread_mutex_unlock(&mutex);
}

void return_forks(int phnum) {
    pthread_mutex_lock(&mutex);

    state[phnum] = THINKING;
    printf("Philosopher %d putting fork %d and %d down (THINKING)\n", phnum + 1, LEFT + 1, phnum + 1);

    test(LEFT);
    test(RIGHT);

    pthread_mutex_unlock(&mutex);
}

// Өгсөн хугацаанд (миллисекундээр) хүлээнэ. (Философичийн бодох, хооллох хугацаа)
void sleep_ms(int ms) {
    sleep(ms / 1000);
    usleep((ms % 1000) * 1000);
}

// Философич бүрийн ажиллах Thread функц
void* philosopher(void* num) {
    int* i = num;

    while (1) {
        sleep_ms(think_ms);             // Сэтгэн бодох хугацаа (THINKING)
        pickup_forks(*i);
        sleep_ms(eat_ms);               // Хоол идэх хугацаа (EATING)
        return_forks(*i);
    }
}

// [QA] Parses an argument as an integer and checks the range.
// Prints an error and returns 0 if invalid.
int parse_arg(const char* text, const char* name, long min, long max, int* out) {
    char* end;
    long v = strtol(text, &end, 10);

    if (end == text || *end != '\0') {
        fprintf(stderr, "Error: %s must be an integer (got \"%s\").\n", name, text);
        return 0;
    }
    if (v < min || v > max) {
        fprintf(stderr, "Error: %s must be between %ld and %ld (got %ld).\n", name, min, max, v);
        return 0;
    }
    *out = (int)v;
    return 1;
}

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);   // [QA] unbuffered output so server.py reads lines immediately

    // Usage: ./philosophers [n] [thinkMs] [eatMs]
    if (argc > 4) {
        fprintf(stderr, "Usage: %s [n] [thinkMs] [eatMs]\n", argv[0]);   // [QA]
        return 1;
    }
    if (argc > 1 && !parse_arg(argv[1], "Number of philosophers", 2, MAX_N, &N)) return 1;   // [QA]
    if (argc > 2 && !parse_arg(argv[2], "Think time (ms)", 100, 10000, &think_ms)) return 1; // [QA]
    if (argc > 3 && !parse_arg(argv[3], "Eat time (ms)", 100, 10000, &eat_ms)) return 1;     // [QA]

    pthread_t thread_id[MAX_N];

    pthread_mutex_init(&mutex, NULL);
    for (int i = 0; i < N; i++) {
        pthread_cond_init(&cond[i], NULL);
        phil[i] = i;
        state[i] = THINKING;
    }

    for (int i = 0; i < N; i++) {
        pthread_create(&thread_id[i], NULL, philosopher, &phil[i]);
        printf("Philosopher %d is THINKING\n", i + 1);
    }

    for (int i = 0; i < N; i++) {
        pthread_join(thread_id[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    for (int i = 0; i < N; i++) {
        pthread_cond_destroy(&cond[i]);
    }

    return 0;
}
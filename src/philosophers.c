#include <stdio.h>
#include <stdlib.h>
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
int eat_ms = 2000; 

pthread_mutex_t mutex;
pthread_cond_t cond[MAX_N];

// Хоёр талын философич идэхгүй байгаа бөгөөд одоогийн философич өлсөж байвал идэхийг зөвшөөрөх
void test(int phnum) {
    if (state[phnum] == HUNGRY &&
        state[LEFT] != EATING &&
        state[RIGHT] != EATING) {
        
        state[phnum] = EATING;
        printf("Philosopher %d takes fork %d and %d (EATING)\n", phnum + 1, LEFT + 1, phnum + 1);
        
        // Хүлээж байсан condition variable-ийг сэрээнэ
        pthread_cond_signal(&cond[phnum]);
    }
}

// Сэрээ авах функц
void pickup_forks(int phnum) {
    pthread_mutex_lock(&mutex);

    state[phnum] = HUNGRY;
    printf("Philosopher %d is HUNGRY\n", phnum + 1);

    // Хажуугийн хүмүүс идэж байвал хүлээнэ
    test(phnum);

    while (state[phnum] != EATING) {
        pthread_cond_wait(&cond[phnum], &mutex);
    }

    pthread_mutex_unlock(&mutex);
}

// Сэрээ буцааж тавих функц
void return_forks(int phnum) {
    pthread_mutex_lock(&mutex);

    state[phnum] = THINKING;
    printf("Philosopher %d putting fork %d and %d down (THINKING)\n", phnum + 1, LEFT + 1, phnum + 1);

    // Баруун болон зүүн талын философичид идэх боломжтой болсныг шалгаж сэрээнэ
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
        sleep(think_ms);             // Сэтгэн бодох хугацаа (THINKING)
        pickup_forks(*i);
        sleep(eat_ms);             // Хоол идэх хугацаа (EATING)
        return_forks(*i);
    }
}

// Аргументыг бүхэл тоо болгож шалгана. Буруу бол алдаа хэвлээд 0 буцаана
int parse_arg(const char* text, const char* name, long min, long max, int* out) {
    char* end;
    long v = strtol(text, &end, 10);

    if (end == text || *end != '\0') {
        fprintf(stderr, "Алдаа: %s бүхэл тоо байх ёстой (өгсөн утга: \"%s\").\n", name, text);
        return 0;
    }
    if (v < min || v > max) {
        fprintf(stderr, "Алдаа: %s нь %ld-%ld хооронд байх ёстой (өгсөн утга: %ld).\n", name, min, max, v);
        return 0;
    }
    *out = (int)v;
    return 1;
}

int main(int argc, char* argv[]) {
    // Хэрэглээ: ./philosophers [n] [thinkMs] [eatMs]
    if (argc > 4) {
        fprintf(stderr, "Хэрэглээ: %s [n] [thinkMs] [eatMs]\n", argv[0]);
        return 1;
    }
    if (argc > 1 && !parse_arg(argv[1], "n (философичдын тоо)", 2, MAX_N, &N)) return 1;
    if (argc > 2 && !parse_arg(argv[2], "thinkMs (бодох хугацаа)", 100, 10000, &think_ms)) return 1;
    if (argc > 3 && !parse_arg(argv[3], "eatMs (хооллох хугацаа)", 100, 10000, &eat_ms)) return 1;


    pthread_t thread_id[MAX_N];

    // Mutex болон Condition Variables идэвхжүүлэх
    pthread_mutex_init(&mutex, NULL);
    for (int i = 0; i < N; i++) {
        pthread_cond_init(&cond[i], NULL);
        phil[i] = i;
        state[i] = THINKING;
    }

    // Thread-үүдийг үүсгэх
    for (int i = 0; i < N; i++) {
        pthread_create(&thread_id[i], NULL, philosopher, &phil[i]);
        printf("Philosopher %d is THINKING\n", i + 1);
    }

    // Thread-үүд дуусахыг хүлээх
    for (int i = 0; i < N; i++) {
        pthread_join(thread_id[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    for (int i = 0; i < N; i++) {
        pthread_cond_destroy(&cond[i]);
    }

    return 0;
}
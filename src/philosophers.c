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

// Философич бүрийн ажиллах Thread функц
void* philosopher(void* num) {
    int* i = num;

    while (1) {
        sleep(1);             // Сэтгэн бодох хугацаа (THINKING)
        pickup_forks(*i);
        sleep(2);             // Хоол идэх хугацаа (EATING)
        return_forks(*i);
    }
}

int main() {
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
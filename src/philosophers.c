#include <stdio.h>
#include <stdlib.h>
#include <errno.h>          
#include <pthread.h>
#include <unistd.h>

#define THINKING 0          
#define HUNGRY 1
#define EATING 2

#define MAX_N 20            
#define MIN_N 2
#define MIN_MS 1
#define MAX_MS 10000

int N = 5;                 
int think_ms = 1000;        
int eat_ms = 2000;          

#define LEFT (phnum + N - 1) % N
#define RIGHT (phnum + 1) % N

int state[MAX_N];           
int phil[MAX_N];

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

void msleep(int ms) {
    sleep(ms / 1000);
    usleep((ms % 1000) * 1000);
}

void* philosopher(void* num) {
    int* i = num;

    while (1) {
        msleep(think_ms);     
        pickup_forks(*i);
        msleep(eat_ms);       
        return_forks(*i);
    }
}

int parse_arg(const char* s, const char* name, int min, int max) {
    char* end;
    errno = 0;
    long v = strtol(s, &end, 10);

    if (s[0] == '\0' || *end != '\0' || errno == ERANGE) {
        fprintf(stderr, "Error: %s must be an integer (got \"%s\")\n", name, s);
        exit(1);
    }
    if (v < min || v > max) {
        fprintf(stderr, "Error: %s must be between %d and %d (got %ld)\n", name, min, max, v);
        exit(1);
    }
    return (int)v;
}

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0); 

    if (argc > 4) {
        fprintf(stderr, "Usage: %s [N] [think_ms] [eat_ms]\n", argv[0]);
        return 1;
    }
    if (argc > 1) N        = parse_arg(argv[1], "Number of philosophers", MIN_N, MAX_N);
    if (argc > 2) think_ms = parse_arg(argv[2], "Think time (ms)", MIN_MS, MAX_MS);
    if (argc > 3) eat_ms   = parse_arg(argv[3], "Eat time (ms)", MIN_MS, MAX_MS);

    pthread_t thread_id[MAX_N];

    pthread_mutex_init(&mutex, NULL);
    for (int i = 0; i < N; i++) {
        pthread_cond_init(&cond[i], NULL);
        state[i] = THINKING;
        phil[i] = i;                   
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
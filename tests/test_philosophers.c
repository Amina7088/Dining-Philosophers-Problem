/* tests/test_philosophers.c
 * Unit, scaling and stress tests for src/philosophers.c (source is not modified).
 * main() of the source is renamed so the file can be #included.
 *
 * Compile (from the repo root):
 *     gcc tests\test_philosophers.c -o test_philosophers -pthread
 * Run:
 *     .\test_philosophers.exe
 * Output: terminal and tests/results.txt
 */
#define main philosophers_main
#include "../src/philosophers.c"
#undef main

#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
#define NULL_DEVICE "NUL"
#else
#define NULL_DEVICE "/dev/null"
#endif

/* ---------- output: terminal + file ---------- */
static FILE* report = NULL;
static FILE* console = NULL;    /* where test output goes */

static void say(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt); vfprintf(console ? console : stderr, fmt, ap); va_end(ap);
    if (report) { va_start(ap, fmt); vfprintf(report, fmt, ap); va_end(ap); }
}

/* ---------- tiny test framework ---------- */
static int failures = 0, checks = 0, group_start = 0;
static const char* group_name = "";

#define CHECK(cond, msg) do { \
    checks++; \
    if (!(cond)) { failures++; say("    FAIL: %s (line %d)\n", (msg), __LINE__); } \
} while (0)

static void begin(const char* name) { group_name = name; group_start = failures; }
static void end_group(void) {
    say("[%s] %s\n", failures == group_start ? "PASS" : "FAIL", group_name);
}

static long long now_ms(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void reset_state(int n) {
    N = n;
    for (int i = 0; i < MAX_N; i++) state[i] = THINKING;
}

/* ---------- 1. parse_arg ---------- */
static void test_parse_arg(void) {
    int out;
    begin("parse_arg(): valid and invalid values");

    const char* ok_n[] = {"2", "5", "10"};
    for (int k = 0; k < 3; k++) {
        out = -1;
        CHECK(parse_arg(ok_n[k], "n", 2, MAX_N, &out) == 1, "valid N accepted");
        CHECK(out == atoi(ok_n[k]), "valid N stored");
    }

    const char* bad_n[] = {"1", "0", "-3", "11", "100", "abc", "3.5", "", "5 ", "99999999999"};
    for (int k = 0; k < 10; k++) {
        out = -1;
        CHECK(parse_arg(bad_n[k], "n", 2, MAX_N, &out) == 0, "invalid N rejected");
        CHECK(out == -1, "output untouched when rejected");
    }

    const char* ok_t[] = {"100", "1000", "10000"};
    for (int k = 0; k < 3; k++)
        CHECK(parse_arg(ok_t[k], "t", 100, 10000, &out) == 1, "valid time accepted");

    const char* bad_t[] = {"99", "0", "-1", "10001", "abc", "2.5", ""};
    for (int k = 0; k < 7; k++)
        CHECK(parse_arg(bad_t[k], "t", 100, 10000, &out) == 0, "invalid time rejected");

    end_group();
}

/* ---------- 2. main(): invalid arguments return 1 ---------- */
static void test_main_args(void) {
    begin("main(): invalid arguments return 1");
    char* cases[][6] = {
        {"p", "1", NULL},
        {"p", "0", NULL},
        {"p", "11", NULL},
        {"p", "abc", NULL},
        {"p", "3.5", NULL},
        {"p", "5", "99", NULL},
        {"p", "5", "10001", NULL},
        {"p", "5", "200", "99", NULL},
        {"p", "5", "200", "200", "7", NULL},   /* too many arguments */
    };
    int total = (int)(sizeof(cases) / sizeof(cases[0]));
    for (int k = 0; k < total; k++) {
        int argc = 0;
        while (cases[k][argc] != NULL) argc++;
        N = 5; think_ms = 1000; eat_ms = 2000;
        char msg[64];
        snprintf(msg, sizeof msg, "invalid args case #%d must return 1", k + 1);
        CHECK(philosophers_main(argc, cases[k]) == 1, msg);
    }
    end_group();
}

/* ---------- 3. test() and return_forks() logic ---------- */
static void test_logic(void) {
    begin("test()/return_forks(): eating rules");

    reset_state(5); state[2] = HUNGRY; test(2);
    CHECK(state[2] == EATING, "hungry with free neighbors starts eating");

    reset_state(5); state[2] = HUNGRY; state[1] = EATING; test(2);
    CHECK(state[2] == HUNGRY, "left neighbor eating -> must wait");

    reset_state(5); state[2] = HUNGRY; state[3] = EATING; test(2);
    CHECK(state[2] == HUNGRY, "right neighbor eating -> must wait");

    reset_state(5); state[0] = HUNGRY; state[4] = EATING; test(0);
    CHECK(state[0] == HUNGRY, "wrap-around: philosopher 1 vs 5");

    reset_state(5); state[4] = HUNGRY; state[0] = EATING; test(4);
    CHECK(state[4] == HUNGRY, "wrap-around: philosopher 5 vs 1");

    reset_state(5); state[2] = THINKING; test(2);
    CHECK(state[2] == THINKING, "thinking philosopher is not woken");

    reset_state(2); state[0] = HUNGRY; state[1] = EATING; test(0);
    CHECK(state[0] == HUNGRY, "N=2: must wait for the only neighbor");

    reset_state(2); state[0] = HUNGRY; test(0);
    CHECK(state[0] == EATING, "N=2: eats when neighbor is free");

    reset_state(5);
    state[1] = EATING; state[0] = HUNGRY; state[2] = HUNGRY;
    return_forks(1);
    CHECK(state[1] == THINKING, "return_forks: back to THINKING");
    CHECK(state[0] == EATING && state[2] == EATING, "return_forks: both waiting neighbors start eating");

    end_group();
}

/* ---------- 4. Stress runs with measurements ---------- */
typedef struct {
    long total;
    int violations;
    int max_eating;
    long long max_wait;
    int min_meals, max_meals;
    int deadlock;
} Result;

static int w_think, w_eat;
static volatile int run_flag;
static int eating_now, max_eating, violations;
static volatile long ate[MAX_N];
static long long wait_max[MAX_N];

static void* worker(void* p) {
    int i = *(int*)p;
    while (run_flag) {
        sleep_ms(w_think);                          /* THINKING */
        long long t0 = now_ms();
        pickup_forks(i);
        long long w = now_ms() - t0;                /* wait: HUNGRY -> EATING */
        if (w > wait_max[i]) wait_max[i] = w;

        pthread_mutex_lock(&mutex);
        eating_now++;
        if (eating_now > max_eating) max_eating = eating_now;
        if (state[i] != EATING ||
            state[(i + N - 1) % N] == EATING ||
            state[(i + 1) % N] == EATING)
            violations++;                           /* neighbors ate together */
        pthread_mutex_unlock(&mutex);

        sleep_ms(w_eat);                            /* EATING */
        ate[i]++;
        pthread_mutex_lock(&mutex);
        eating_now--;
        pthread_mutex_unlock(&mutex);
        return_forks(i);
    }
    return NULL;
}

static void stress_run(int n, int t_ms, int e_ms, int seconds, Result* r) {
    reset_state(n);
    w_think = t_ms; w_eat = e_ms;
    eating_now = 0; max_eating = 0; violations = 0;
    for (int i = 0; i < MAX_N; i++) { ate[i] = 0; wait_max[i] = 0; }
    run_flag = 1;

    pthread_t th[MAX_N];
    for (int i = 0; i < n; i++) {
        phil[i] = i;
        pthread_create(&th[i], NULL, worker, &phil[i]);
    }

    /* Check progress every 0.5 s. No progress means deadlock. */
    long prev = 0;
    r->deadlock = 0;
    for (int s = 0; s < seconds * 2; s++) {
        usleep(500000);
        long now = 0;
        for (int i = 0; i < n; i++) now += ate[i];
        if (now <= prev) { r->deadlock = 1; return; }   /* threads stuck, no join */
        prev = now;
    }

    run_flag = 0;
    for (int i = 0; i < n; i++) pthread_join(th[i], NULL);

    r->total = 0; r->max_wait = 0;
    r->min_meals = (int)ate[0]; r->max_meals = (int)ate[0];
    for (int i = 0; i < n; i++) {
        r->total += ate[i];
        if (wait_max[i] > r->max_wait) r->max_wait = wait_max[i];
        if (ate[i] < r->min_meals) r->min_meals = (int)ate[i];
        if (ate[i] > r->max_meals) r->max_meals = (int)ate[i];
    }
    r->violations = violations;
    r->max_eating = max_eating;
}

static void die_deadlock(void) {
    say("    FAIL: no progress for 0.5 s: DEADLOCK detected\n");
    say("\nStopped: threads are stuck, cannot continue.\n");
    if (report) fclose(report);
    exit(1);
}

static void check_result(int n, const Result* r, int seconds) {
    CHECK(r->violations == 0, "two neighbors ate at the same time");
    CHECK(r->max_eating <= n / 2, "more than N/2 philosophers ate at once");
    CHECK(r->min_meals > 0, "a philosopher never ate (starvation)");
    CHECK(r->max_wait < seconds * 1000LL, "a philosopher waited the whole run (starvation)");
}

/* Scaling with N: this table goes into the report */
static void test_scaling(void) {
    begin("scaling table (think=20ms, eat=20ms, 3 s per N)");
    say("    N | meals | viol | max_eat | N/2 | meals min..max | max_wait_ms\n");
    int ns[] = {2, 3, 5, 8, 10};
    for (int k = 0; k < 5; k++) {
        int n = ns[k];
        Result r;
        stress_run(n, 20, 20, 3, &r);
        if (r.deadlock) die_deadlock();
        check_result(n, &r, 3);
        say("    %2d | %5ld | %4d | %7d | %3d | %6d..%-6d | %lld\n",
            n, r.total, r.violations, r.max_eating, n / 2, r.min_meals, r.max_meals, r.max_wait);
    }
    end_group();
}

/* Very short times: tries to expose races and deadlocks */
static void test_stress(void) {
    begin("stress: very short think/eat times");
    int cfg[][3] = {{10, 1, 1}, {7, 1, 10}, {2, 1, 1}, {3, 1, 1}};
    for (int k = 0; k < 4; k++) {
        Result r;
        stress_run(cfg[k][0], cfg[k][1], cfg[k][2], 3, &r);
        if (r.deadlock) die_deadlock();
        check_result(cfg[k][0], &r, 3);
        say("    N=%d think=%dms eat=%dms: meals=%ld viol=%d max_eat=%d max_wait=%lldms\n",
            cfg[k][0], cfg[k][1], cfg[k][2], r.total, r.violations, r.max_eating, r.max_wait);
    }
    end_group();
}

/* Repeat: races are rare, so run many times */
static void test_repeat(void) {
    begin("repeat: 15 short runs (N=7, think=1ms, eat=5ms)");
    int bad = 0;
    for (int k = 0; k < 15; k++) {
        Result r;
        stress_run(7, 1, 5, 1, &r);
        if (r.deadlock) die_deadlock();
        if (r.violations != 0 || r.max_eating > 3 || r.min_meals == 0) bad++;
    }
    CHECK(bad == 0, "some repeated runs had violations or starvation");
    say("    runs with problems: %d / 15\n", bad);
    end_group();
}

/* ---------- main ---------- */
int main(void) {
    console = fdopen(dup(fileno(stderr)), "w");   /* keep the terminal for test output */
    setvbuf(console, NULL, _IONBF, 0);
    freopen(NULL_DEVICE, "w", stdout);            /* hide source printf output */
    freopen(NULL_DEVICE, "w", stderr);            /* hide source error messages (Mongolian) */
    report = fopen("tests/results.txt", "w");

    pthread_mutex_init(&mutex, NULL);
    for (int i = 0; i < MAX_N; i++) pthread_cond_init(&cond[i], NULL);

    say("philosophers.c tests (limits: N 2-%d, time 100-10000 ms)\n", MAX_N);
    say("(parse_arg error messages are hidden in this output)\n\n");

    test_parse_arg();
    test_main_args();
    test_logic();
    test_scaling();
    test_stress();
    test_repeat();

    say("\n%d checks, %d failed\n", checks, failures);
    say(failures == 0 ? "ALL TESTS PASSED\n" : "SOME TESTS FAILED\n");
    if (report) fclose(report);
    return failures == 0 ? 0 : 1;
}
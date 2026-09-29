#include "pthreadfuncs.h"

int g_fd = -1;
pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_counter_lock = PTHREAD_MUTEX_INITIALIZER;
int g_counter = 0;

static int  pc_buffer = 0;
static int  pc_has_data = 0;
static int  pc_done = 0;
static pthread_mutex_t pc_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  pc_cond = PTHREAD_COND_INITIALIZER;
#define PC_ITEMS 5

pid_t getThreadID(void) {
    return (pid_t) syscall(SYS_gettid);
}

int write_line(const char *msg) {
    int use_mutex = (getenv("NO_LOG_MUTEX") == NULL);
    if (use_mutex) pthread_mutex_lock(&g_lock);
    ssize_t n = write(g_fd, msg, strlen(msg));
    if (use_mutex) pthread_mutex_unlock(&g_lock);
    if (n < 0) {
        fprintf(stderr, "write() failed: %s, [file descr = %d]\n", strerror(errno), g_fd);
        return -1;
    }
    return 0;
}

void *func_thread(void *arg) {
    struct ThreadArgs *t = (struct ThreadArgs *)arg;
    char buf[192];
    long sleep_ms = 100;
    const char *sleep_env = getenv("SLEEP_MS");
    if (sleep_env) sleep_ms = atol(sleep_env);

    for (int i = 0; i < COUNT_ITERATIONS; ++i) {
        snprintf(buf, sizeof(buf),
            "[tag = %s] pid = %d ppid = %d tid = %d pthread_self = %lu iter = %d msg = %s\n",
            t->tag, getpid(), getppid(), getThreadID(),
            (unsigned long)pthread_self(), i, t->message);
        write_line(buf);
        struct timespec ts = {.tv_sec = sleep_ms / 1000, .tv_nsec = (sleep_ms % 1000) * 1000000L};
        nanosleep(&ts, NULL);
    }
    return NULL;
}

void *func_thread_detach(void *arg) {
    (void)arg;
    pthread_detach(pthread_self());
    char buf[128];
    snprintf(buf, sizeof(buf), "[detached] tid = %d: сам себя detach через pthread_detach(pthread_self())\n", getThreadID());
    write_line(buf);
    return NULL;
}

void *func_thread_exit(void *arg) {
    (void)arg;
    char buf[128];
    snprintf(buf, sizeof(buf), "[exit-demo] tid = %d: завершаюсь через pthread_exit((void*)42L)\n", getThreadID());
    write_line(buf);
    pthread_exit((void *)42L);
}

void *func_thread_malloc_return(void *arg) {
    int iters = *(int *)arg;
    char *result = malloc(64);
    if (result) snprintf(result, 64, "поток отработал %d итераций", iters);
    char buf[128];
    snprintf(buf, sizeof(buf), "[malloc-return] tid = %d: возвращаю malloc-строку\n", getThreadID());
    write_line(buf);
    return result;
}

void *func_thread_malloc_detached(void *arg) {
    struct ThreadArgs *t = (struct ThreadArgs *)arg;
    char buf[160];
    snprintf(buf, sizeof(buf), "[malloc-detached] tid = %d tag = %s: работаю со структурой из malloc\n", getThreadID(), t->tag);
    write_line(buf);
    free(t);
    return NULL;
}

void *func_thread_counter(void *arg) {
    (void)arg;
    int iters = 100000;
    const char *iter_env = getenv("ITER");
    if (iter_env) iters = atoi(iter_env);
    int use_mutex = (getenv("NO_COUNTER_MUTEX") == NULL);
    for (int i = 0; i < iters; ++i) {
        if (use_mutex) pthread_mutex_lock(&g_counter_lock);
        g_counter++;
        if (use_mutex) pthread_mutex_unlock(&g_counter_lock);
    }
    return NULL;
}

void *func_producer(void *arg) {
    (void)arg;
    for (int i = 1; i <= PC_ITEMS; ++i) {
        pthread_mutex_lock(&pc_lock);
        while (pc_has_data) pthread_cond_wait(&pc_cond, &pc_lock);
        pc_buffer = i;
        pc_has_data = 1;
        pthread_cond_signal(&pc_cond);
        pthread_mutex_unlock(&pc_lock);
        struct timespec ts = {.tv_sec = 0, .tv_nsec = 50L * 1000L * 1000L};
        nanosleep(&ts, NULL);
    }
    pthread_mutex_lock(&pc_lock);
    pc_done = 1;
    pthread_cond_broadcast(&pc_cond);
    pthread_mutex_unlock(&pc_lock);
    return NULL;
}

void *func_consumer(void *arg) {
    (void)arg;
    char buf[96];
    for (;;) {
        pthread_mutex_lock(&pc_lock);
        while (!pc_has_data && !pc_done) pthread_cond_wait(&pc_cond, &pc_lock);
        if (!pc_has_data && pc_done) { pthread_mutex_unlock(&pc_lock); break; }
        int value = pc_buffer;
        pc_has_data = 0;
        pthread_cond_signal(&pc_cond);
        pthread_mutex_unlock(&pc_lock);
        snprintf(buf, sizeof(buf), "[producer-consumer] потребитель получил значение %d\n", value);
        write_line(buf);
    }
    write_line("[producer-consumer] потребитель завершил работу\n");
    return NULL;
}

void about(void) {
    printf("Pthread example\n");
}
// правка

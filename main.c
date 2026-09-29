#include "pthreadfuncs.h"
#include <stdio.h>

int main(void) {
    g_fd = open("output.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (g_fd < 0) { perror("open output.log"); return 1; }

    char buf[128];

    if (getenv("ONLY_COUNTER")) {
        pthread_t counters[COUNT_THREADS];
        g_counter = 0;
        for (int i = 0; i < COUNT_THREADS; ++i) pthread_create(&counters[i], NULL, func_thread_counter, NULL);
        for (int i = 0; i < COUNT_THREADS; ++i) pthread_join(counters[i], NULL);
        printf("g_counter = %d (мьютекс %s)\n", g_counter, getenv("NO_COUNTER_MUTEX") ? "выключен" : "включён");
        close(g_fd);
        return 0;
    }

    snprintf(buf, sizeof(buf), "main: pid = %d, ppid = %d\n", getpid(), getppid());
    write_line(buf);
    printf("main: pid = %d, ppid = %d\n", getpid(), getppid());

    pthread_t threads[COUNT_THREADS];
    struct ThreadArgs args[COUNT_THREADS];
    for (int i = 0; i < COUNT_THREADS; ++i) {
        args[i].id = i;
        snprintf(args[i].tag, sizeof(args[i].tag), "T%d", i);
        snprintf(args[i].message, sizeof(args[i].message), "привет от потока %d", i);
        pthread_create(&threads[i], NULL, func_thread, &args[i]);
    }

    if (getenv("NO_JOIN")) {
        write_line("main: NO_JOIN=1, pthread_join пропущен намеренно\n");
        close(g_fd);
        return 0;
    }

    for (int i = 0; i < COUNT_THREADS; ++i) pthread_join(threads[i], NULL);

    pthread_t td;
    pthread_create(&td, NULL, func_thread_detach, NULL);
    struct timespec pause = {.tv_sec = 0, .tv_nsec = 300L * 1000L * 1000L};
    nanosleep(&pause, NULL);
    int rc = pthread_join(td, NULL);
    snprintf(buf, sizeof(buf), "main: pthread_join на detached-потоке вернул rc=%d (%s)\n", rc, strerror(rc));
    write_line(buf);

    pthread_t te; void *ret = NULL;
    pthread_create(&te, NULL, func_thread_exit, NULL);
    pthread_join(te, &ret);
    snprintf(buf, sizeof(buf), "main: поток вернул через pthread_exit значение %ld\n", (long)ret);
    write_line(buf);

    pthread_t tm; int iters_for_malloc = COUNT_ITERATIONS; void *mret = NULL;
    pthread_create(&tm, NULL, func_thread_malloc_return, &iters_for_malloc);
    pthread_join(tm, &mret);
    if (mret) {
        snprintf(buf, sizeof(buf), "main: получена malloc-строка: %s\n", (char *)mret);
        write_line(buf);
        free(mret);
    }

    struct ThreadArgs *heap_args = malloc(sizeof(struct ThreadArgs));
    heap_args->id = 99;
    snprintf(heap_args->tag, sizeof(heap_args->tag), "HEAP");
    pthread_t tdm;
    pthread_create(&tdm, NULL, func_thread_malloc_detached, heap_args);
    pthread_detach(tdm);

    pthread_t counters[COUNT_THREADS];
    g_counter = 0;
    for (int i = 0; i < COUNT_THREADS; ++i) pthread_create(&counters[i], NULL, func_thread_counter, NULL);
    for (int i = 0; i < COUNT_THREADS; ++i) pthread_join(counters[i], NULL);
    snprintf(buf, sizeof(buf), "main: итоговое g_counter = %d\n", g_counter);
    write_line(buf);

    pthread_t prod, cons;
    pthread_create(&cons, NULL, func_consumer, NULL);
    pthread_create(&prod, NULL, func_producer, NULL);
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    close(g_fd);
    return 0;
}

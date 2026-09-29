#ifndef PTHREADFUNCS_H
#define PTHREADFUNCS_H

#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define COUNT_THREADS 4
#define COUNT_ITERATIONS 3

struct ThreadArgs {
    int  id;
    char tag[10];
    char message[64];
};

extern int g_fd;
extern pthread_mutex_t g_lock;
extern pthread_mutex_t g_counter_lock;
extern int g_counter;

pid_t getThreadID(void);
int   write_line(const char *msg);
void *func_thread(void *arg);
void *func_thread_detach(void *arg);
void *func_thread_exit(void *arg);
void *func_thread_malloc_return(void *arg);
void *func_thread_malloc_detached(void *arg);
void *func_thread_counter(void *arg);
void *func_producer(void *arg);
void *func_consumer(void *arg);
void  about(void);

#endif

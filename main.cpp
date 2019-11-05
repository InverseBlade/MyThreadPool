#include "ThreadPool.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

typedef struct _args {
    int taskId;
    int data;
    int runtime;
} Args;

pthread_mutex_t mtx_count;

void callback(void* data)
{
    Args* args = (Args*)data;
    usleep(args->runtime * 100);
    printf("TaskId:\t%d in Thread:\t%d spends\t%ds.\n", args->taskId, pthread_self(), args->runtime);
    fflush(stdout);
    delete data;
}

int main(int argc, char** argv)
{
    ThreadPool threadpool(60, 5000);
    clock_t startTime = clock();
    pthread_mutex_init(&mtx_count, NULL);

    threadpool.start();
    srand(time(nullptr));
    for(int i = 0; i < 2000; i++) {
        Args* args = new Args;
        args->taskId = i;
        args->runtime = 1500;
        threadpool.addTask(&callback, args);
    }
    threadpool.destroy(GRACEFUL_SHUT_DOWN);
    pthread_mutex_destroy(&mtx_count);
    printf("time spend=%d\n", clock() - startTime);
    scanf("%s", nullptr);
    return 0;
}

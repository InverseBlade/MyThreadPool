#ifndef _THREAD_POclTabCtrlOL_H
#define _THREAD_POOL_H

#include <pthread.h>
#include <vector>

const int THREAD_POOL_LOCK_FAILURE = -1;
const int THREAD_POOL_QUEUE_FULL = 0;
const int THREAD_POOL_SHUTDOWN = 1;
const int THREAD_POOL_FAILURE = 2;

typedef enum { IMMEDIATE_SHUT_DOWN = 1, GRACEFUL_SHUT_DOWN = 2 } ShutDownOpt;

typedef struct _threadPoolTask {
    void (*callback)(void* args);
    void* args;
} ThreadPoolTask;

// Task callback function protocol
typedef void (*CallBack)(void* args);

// Thread Worker
void* worker(void* args);

class ThreadPool
{
public:
    ThreadPool(int workerCount, int queueSize);
    int start();
    int addTask(void (*func)(void*), void* args);
    int destroy(ShutDownOpt opt = GRACEFUL_SHUT_DOWN);

public:
    // friend function
    friend void* worker(void* args);

private:
    pthread_mutex_t lock;
    pthread_cond_t notify;

    std::vector<pthread_t> workers;
    std::vector<ThreadPoolTask> queueTask;

    int workerCount;
    int queueSize;
    int queueHead;
    int queueTail;
    int taskCount;
    bool started;
    ShutDownOpt shutdown;
};

#endif

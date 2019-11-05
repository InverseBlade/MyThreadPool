#include "ThreadPool.h"
#include "assert.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>

ThreadPool::ThreadPool(int workerCount, int queueSize)
    : workerCount(workerCount)
    , queueSize(queueSize)
    , queueHead(0)
    , queueTail(0)
    , taskCount(0)
    , started(false)
    , shutdown(GRACEFUL_SHUT_DOWN)
{
    workers.resize(workerCount);
    queueTask.resize(queueSize);
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&notify, NULL);
}

// thread worker funciton
void* worker(void* args)
{
    ThreadPool* that = (ThreadPool*)args;
    while(true) {
        pthread_mutex_lock(&(that->lock));
        // worker sleep when task queue is empty
        while(that->taskCount == 0 && that->started) {
            // sleep
            pthread_cond_wait(&that->notify, &that->lock);
        }
        if((!that->started && that->shutdown == IMMEDIATE_SHUT_DOWN) ||
            (!that->started && that->shutdown == GRACEFUL_SHUT_DOWN && that->taskCount == 0)) {
            break;
        }
        // take the task
        ThreadPoolTask task = that->queueTask[that->queueHead];
        that->queueHead = (that->queueHead + 1) % that->queueSize;
        that->taskCount--;
        pthread_mutex_unlock(&that->lock);
        // run task
        (*task.callback)(task.args);
    }
    pthread_mutex_unlock(&that->lock);
    printf("Thread\t%d exit\n", pthread_self());
    pthread_exit(nullptr);
    return nullptr;
}

int ThreadPool::start()
{
    clock_t start_tm = clock();
    pthread_mutex_lock(&lock);
    for(int i = 0; i < workers.size(); i++) {
        pthread_t thread_id;
        if(pthread_create(&thread_id, NULL, &worker, (void*)this) != 0) {
            assert("pthread_create ERROR!");
            return -1;
        }
        workers[i] = thread_id;
    }
    this->started = true;
    pthread_mutex_unlock(&lock);
    printf("thread pool started! used time=%d.\n", clock() - start_tm);
    return 0;
}

int ThreadPool::addTask(void (*callback)(void*), void* args)
{
    int err = 0;
    do {
        if(pthread_mutex_lock(&lock) != 0) {
            err = THREAD_POOL_LOCK_FAILURE;
            break;
        }
        // queue full
        if(taskCount == queueSize) {
            err = THREAD_POOL_QUEUE_FULL;
            break;
        }
        if(!started) {
            err = THREAD_POOL_SHUTDOWN;
            break;
        }
        ThreadPoolTask task;
        task.callback = callback;
        task.args = args;
        // put task into queue
        queueTask[queueTail] = task;
        queueTail = (queueTail + 1) % queueSize;
        taskCount++;
        // notify a worker
        if(pthread_cond_signal(&notify) != 0) {
            err = THREAD_POOL_LOCK_FAILURE;
            break;
        }
    } while(false);
    // release the lock
    if(pthread_mutex_unlock(&lock) != 0)
        err = THREAD_POOL_LOCK_FAILURE;
    return err;
}

int ThreadPool::destroy(ShutDownOpt opt)
{
    int err = 0;
    do {
        pthread_mutex_lock(&lock);
        started = false;
        shutdown = opt;
        pthread_cond_broadcast(&notify);
        pthread_mutex_unlock(&lock);
        // join
        for(int i = 0; i < workers.size(); i++) {
            pthread_join(workers[i], NULL);
        }
    } while(false);
    pthread_mutex_lock(&lock);
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&notify);
    return 0;
}

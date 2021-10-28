//
// Created by 许斌 on 2021/10/26.
//

#include "ThreadPool/ThreadPool.h"
using namespace xb;

ThreadPool* ThreadPool::instance = nullptr;
pthread_mutex_t ThreadPool::singleton_lock = PTHREAD_MUTEX_INITIALIZER;

ThreadPool::ThreadPool(const int thread_num, const int task_num) :
        MAX_THREAD_NUM(thread_num),
        MAX_TASK_NUM(task_num),
        is_running(false)
{
    pthread_mutex_init(&pool_lock, nullptr);
    pthread_cond_init(&pool_cond, nullptr);
    thread_pool.resize(MAX_THREAD_NUM);
    is_running = true;
    for(int i = 0; i < MAX_THREAD_NUM; i++) {
        pthread_create(&thread_pool[i], nullptr, threadfun, this);
    }
}

ThreadPool::~ThreadPool() {
    if(!is_running) {
        return;
    }
    is_running = false;
    pthread_cond_broadcast(&pool_cond);

    for (int i = 0; i < MAX_THREAD_NUM; i++)
    {
        pthread_join(thread_pool[i], nullptr);
    }
    pthread_mutex_destroy(&pool_lock);
    pthread_cond_destroy(&pool_cond);
}

ThreadPool* ThreadPool::getInstance(const int thread_num,const int task_num) {
    if(instance == nullptr) {
        pthread_mutex_lock(&singleton_lock);
        if(instance == nullptr) {
            instance = new ThreadPool(thread_num, task_num);
        }
    }
    pthread_mutex_unlock(&singleton_lock);
    return instance;
}

int ThreadPool::AddTask(Task task) {
    if(getSize() >= MAX_TASK_NUM) {
        return Full;
    }
    pthread_mutex_lock(&pool_lock);
    task_queue.push_front(task);
    pthread_mutex_unlock(&pool_lock);
    pthread_cond_signal(&pool_cond);
    return Norm;
}

ThreadPool::Task ThreadPool::TakeTask() {
    pthread_mutex_lock(&pool_lock);
    while (task_queue.empty() && is_running) {
        pthread_cond_wait(&pool_cond, &pool_lock);
    }
    Task task = task_queue.front();
    task_queue.pop_front();
    pthread_mutex_unlock(&pool_lock);
    return task;
}

int ThreadPool::getSize() {
    pthread_mutex_lock(&pool_lock);
    int size = task_queue.size();
    pthread_mutex_unlock(&pool_lock);
    return size;
}

void* ThreadPool::threadfun(void* arg) {
    pthread_t tid = pthread_self();
    ThreadPool* pool = static_cast<ThreadPool*>(arg);
    while (pool->is_running) {
        ThreadPool::Task task = pool->TakeTask();
        if (!task) {
            printf("thread %lu will exit\n", tid);
            break;
        }
        task();
    }
}
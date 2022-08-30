//
// Created by 许斌 on 2021/10/26.
//
//#define _DEBUG_
#include <iostream>
#include <memory>
#include "ThreadPool.h"
using namespace xb;

// 构造函数初始化
ThreadPool::ThreadPool() :
        MAX_THREAD_NUM(10),
        MAX_TASK_NUM(20),
        is_running(false)
{
    pthread_mutex_init(&mutex_, nullptr);
    pthread_cond_init(&cond_, nullptr);
    thread_pool.resize(MAX_THREAD_NUM);
    is_running = true;
    // 在线程池内创建线程
    for(int i = 0; i < MAX_THREAD_NUM; i++)
        pthread_create(&thread_pool[i], nullptr, threadfun, this);
}

ThreadPool::ThreadPool(const int thread_num, const int task_num) :
        MAX_THREAD_NUM(thread_num),
        MAX_TASK_NUM(task_num),
        is_running(false)
{
    pthread_mutex_init(&mutex_, nullptr);
    pthread_cond_init(&cond_, nullptr);
    thread_pool.resize(MAX_THREAD_NUM);
    is_running = true;
    // 在线程池内创建线程
    for(int i = 0; i < MAX_THREAD_NUM; i++)
        pthread_create(&thread_pool[i], nullptr, threadfun, this);
}

// 析构函数，销毁instance、互斥锁和条件变量
ThreadPool::~ThreadPool()
{
    stop();
}

void ThreadPool::stop()
{
    if(!is_running)
        return;
    is_running = false;
    for(int i = 0; i < MAX_THREAD_NUM; i++)
        pthread_join(thread_pool[i], nullptr);
}

// 添加任务到任务队列
void ThreadPool::AddTask(const Task& task)
{
    pthread_mutex_lock(&mutex_);
    while(getSize() == MAX_TASK_NUM && is_running)
        pthread_cond_wait(&cond_, &mutex_);
    task_queue.push_front(task);
    pthread_cond_broadcast(&cond_);
    pthread_mutex_unlock(&mutex_);
}

// 从任务队列中提取任务
ThreadPool::Task ThreadPool::TakeTask()
{
    pthread_mutex_lock(&mutex_);
    while (task_queue.empty() && is_running)
        pthread_cond_wait(&cond_, &mutex_);
    Task task = task_queue.front();
    task_queue.pop_front();
    pthread_cond_broadcast(&cond_);
    pthread_mutex_unlock(&mutex_);
    return task;
}

// 获取当前任务队列中任务数
size_t ThreadPool::getSize()
{
    return task_queue.size();
}

// 线程函数，负责执行任务
void* ThreadPool::threadfun(void* arg)
{
    auto pool = static_cast<ThreadPool*>(arg);   // 通过线程参数传递获取线程池实例
    while (pool->is_running)
    {
        ThreadPool::Task task = pool->TakeTask();       // 获取任务
        if (!task)
        {
            break;
        }
#ifdef _DEBUG_
        std::cout << "task: " << pthread_self() << std::endl;
#endif
        task(); // 执行任务
    }
    return nullptr;
}
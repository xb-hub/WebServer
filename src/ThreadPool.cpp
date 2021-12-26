//
// Created by 许斌 on 2021/10/26.
//
//#define _DEBUG_
#include <iostream>
#include <memory>
#include "ThreadPool/ThreadPool.h"
using namespace xb;

std::shared_ptr<ThreadPool> ThreadPool::instance = nullptr;
pthread_mutex_t ThreadPool::singleton_lock = PTHREAD_MUTEX_INITIALIZER;

// 构造函数初始化
ThreadPool::ThreadPool(const int thread_num, const int task_num) :
        MAX_THREAD_NUM(thread_num),
        MAX_TASK_NUM(task_num),
        is_running(false)
{
    pthread_mutex_init(&pool_lock, nullptr);
    pthread_cond_init(&pool_cond, nullptr);
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
    pthread_cond_broadcast(&pool_cond);
    pthread_mutex_destroy(&pool_lock);
    pthread_cond_destroy(&pool_cond);
}

// 单例模式获取instance
std::shared_ptr<ThreadPool> ThreadPool::getInstance(const int thread_num,const int task_num)
{
    if(instance == nullptr)
    {
        pthread_mutex_lock(&singleton_lock);
        if(instance == nullptr)
            // instance = new ThreadPool(thread_num, task_num);
            instance = std::shared_ptr<ThreadPool>(new ThreadPool(thread_num, task_num));
        pthread_mutex_unlock(&singleton_lock);
    }
    return instance;
}

// 添加任务到任务队列
int ThreadPool::AddTask(const Task& task)
{
    if(getSize() >= MAX_TASK_NUM)
        return Full;
    pthread_mutex_lock(&pool_lock);
    task_queue.push_front(task);
    pthread_mutex_unlock(&pool_lock);
    pthread_cond_signal(&pool_cond);
    return Norm;
}

// 从任务队列中提取任务
ThreadPool::Task ThreadPool::TakeTask()
{
    pthread_mutex_lock(&pool_lock);
    while (task_queue.empty() && is_running)
        pthread_cond_wait(&pool_cond, &pool_lock);
    Task task = task_queue.front();
    task_queue.pop_front();
    pthread_mutex_unlock(&pool_lock);
    return task;
}

// 获取当前任务队列中任务数
size_t ThreadPool::getSize()
{
    pthread_mutex_lock(&pool_lock);
    size_t size = task_queue.size();
    pthread_mutex_unlock(&pool_lock);
    return size;
}

// 线程函数，负责执行任务
void* ThreadPool::threadfun(void* arg)
{
    pthread_t tid = pthread_self();
    auto pool = static_cast<ThreadPool*>(arg);   // 通过线程参数传递获取线程池实例
    while (pool->is_running)
    {
        ThreadPool::Task task = pool->TakeTask();       // 获取任务
        if (!task)
        {
            std::cout << "thread " << tid << " will exit" << std::endl;
            break;
        }
#ifdef _DEBUG_
        std::cout << "task: " << pthread_self() << std::endl;
#endif
        task(); // 执行任务
    }
    return nullptr;
}
//
// Created by 许斌 on 2021/10/26.
//
//#define _DEBUG_
#include <memory>
#include "ThreadPool.h"
using namespace xb;

// 构造函数初始化
ThreadPool::ThreadPool() :
        MAX_THREAD_NUM(10),
        MAX_TASK_NUM(20),
        is_running(false),
        cond_(mutex_)
{
    // 在线程池内创建线程
    for(int i = 0; i < MAX_THREAD_NUM; i++)
    {
        pthread_create(thread_pool + i, nullptr, threadfun, this);
        pthread_detach(thread_pool[i]);
    }
    is_running = true;
}

ThreadPool::ThreadPool(const int thread_num, const int task_num) :
        MAX_THREAD_NUM(thread_num),
        MAX_TASK_NUM(task_num),
        is_running(false),
        cond_(mutex_)
{
    thread_pool = new pthread_t[thread_num];
    // 在线程池内创建线程
    for(int i = 0; i < MAX_THREAD_NUM; i++)
    {
        pthread_create(thread_pool + i, nullptr, threadfun, this);
        pthread_detach(thread_pool[i]);
    }
    is_running = true;
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
    delete [] thread_pool;
    is_running = false;
}

// 添加任务到任务队列
void ThreadPool::AddTask(const Task& task)
{
    MutexLockGuard lock(mutex_);
    while(getSize() == MAX_TASK_NUM && is_running)
        cond_.wait();
    task_queue.push_back(task);
    cond_.notifyAll();
}

// 从任务队列中提取任务
ThreadPool::Task ThreadPool::TakeTask()
{
    MutexLockGuard lock(mutex_);
    while (task_queue.empty() && is_running)
        cond_.wait();
    Task task = task_queue.front();
    task_queue.pop_front();
    cond_.notifyAll();
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
            continue;
        }
#ifdef _DEBUG_
        std::cout << "task: " << pthread_self() << std::endl;
#endif
        task(); // 执行任务
    }
    return nullptr;
}
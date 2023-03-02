//
// Created by 许斌 on 2021/10/26.
//
// #define _DEBUG_
#include <memory>
#include <string>
#include <assert.h>
#include "ThreadPool.h"

namespace xb
{
    
// 构造函数初始化
ThreadPool::ThreadPool(const std::string& name) :
        TASK_NUM(20),
        is_running(false),
        mutex_(),
        take_cond_(mutex_),
        add_cond_(mutex_),
        name_(name)
{}

// 析构函数，销毁instance、互斥锁和条件变量
ThreadPool::~ThreadPool()
{
    stop();
}

void ThreadPool::start(int thread_num)
{
    assert(thread_pool.empty());
    is_running = true;
    thread_pool.reserve(thread_num);
    // 在线程池内创建线程
    for(int i = 0; i < thread_num; i++)
    {
        thread_pool.emplace_back(new Thread(name_ + std::to_string(i + 1), std::bind(&ThreadPool::threadfun, this)));
        thread_pool[i]->start();
    }
}

void ThreadPool::stop()
{
    if(!is_running)
        return;
    is_running = false;
    for(auto& it : thread_pool)
    {
        it->join();
    }
}

// 添加任务到任务队列
void ThreadPool::AddTask(const Task& task)
{
// #ifdef _DEBUG_
//     std::cout << "add task!" << std::endl;
// #endif
    MutexLockGuard lock(mutex_);
    while(task_queue.size() >= TASK_NUM && is_running)
        add_cond_.wait();
#ifdef _DEBUG_
    std::cout << task_queue.size() << std::endl;
#endif
    task_queue.push_back(task);
    take_cond_.notifyAll();
}

// 从任务队列中提取任务
ThreadPool::Task ThreadPool::TakeTask()
{
// #ifdef _DEBUG_
//     std::cout << "take task!" << std::endl;
// #endif
    MutexLockGuard lock(mutex_);
    while (task_queue.empty() && is_running)
        take_cond_.wait();
#ifdef _DEBUG_
    std::cout << task_queue.size() << std::endl;
#endif
    Task task = task_queue.front();
    task_queue.pop_front();
    add_cond_.notifyAll();
    return task;
}

// 获取当前任务队列中任务数
size_t ThreadPool::getSize()
{
    MutexLockGuard lock(mutex_);
    return task_queue.size();
}

// 线程函数，负责执行任务
void ThreadPool::threadfun()
{
    while (is_running)
    {
        ThreadPool::Task task = TakeTask();       // 获取任务
        if (!task)
        {
        #ifdef _DEBUG_
            std::cout << "no task" << std::endl;
        #endif
            continue;
        }
        task(); // 执行任务
    }
    return;
}

} // namespace xb

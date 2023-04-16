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
    ThreadPool::ThreadPool(int thread_num)
        : is_running(false),
          m_thread_num(thread_num),
          m_latch(thread_num)
    {
    }

    // 析构函数，销毁instance、互斥锁和条件变量
    ThreadPool::~ThreadPool()
    {
        stop();
    }

    void ThreadPool::start()
    {
        assert(m_thread_pool.empty());
        is_running = true;
        m_thread_pool.reserve(m_thread_num);
        m_event_pool.reserve(m_thread_num);
        // 在线程池内创建线程
        for (int i = 0; i < m_thread_num; i++)
        {
            std::shared_ptr<IOEventLoop> loop = std::make_shared<IOEventLoop>();
            m_thread_pool.emplace_back(std::jthread{std::bind(&IOEventLoop::run, loop.get())});
            m_event_pool.emplace_back(loop);
        }
    }

    void ThreadPool::stop()
    {
        if (!is_running)
            return;
        is_running = false;
    }

    IOEventLoop::ptr ThreadPool::getOneLoopFromPool()
    {
        if(m_thread_pool.empty())
        {

        }
        m_loop_index = m_loop_index + 1 >= m_thread_pool.size() ? 0 : m_loop_index + 1;
        return m_event_pool[m_loop_index];
    }

//     // 添加任务到任务队列
//     void ThreadPool::AddTask(const Task &task)
//     {
//         MutexLockGuard lock(mutex_);
//         while (task_queue.size() >= TASK_NUM && is_running)
//             add_cond_.wait();
// #ifdef _DEBUG_
//         std::cout << task_queue.size() << std::endl;
// #endif
//         task_queue.push_back(task);
//         take_cond_.notifyAll();
//     }

//     // 从任务队列中提取任务
//     ThreadPool::Task ThreadPool::TakeTask()
//     {
//         // #ifdef _DEBUG_
//         //     std::cout << "take task!" << std::endl;
//         // #endif
//         MutexLockGuard lock(mutex_);
//         while (task_queue.empty() && is_running)
//             take_cond_.wait();
// #ifdef _DEBUG_
//         std::cout << task_queue.size() << std::endl;
// #endif
//         Task task = task_queue.front();
//         task_queue.pop_front();
//         add_cond_.notifyAll();
//         return task;
//     }

//     // 获取当前任务队列中任务数
//     size_t ThreadPool::getSize()
//     {
//         MutexLockGuard lock(mutex_);
//         return task_queue.size();
//     }

//     // 线程函数，负责执行任务
//     void ThreadPool::threadfun()
//     {
//         while (is_running)
//         {
//             ThreadPool::Task task = TakeTask(); // 获取任务
//             if (!task)
//             {
// #ifdef _DEBUG_
//                 std::cout << "no task" << std::endl;
// #endif
//                 continue;
//             }
//             task(); // 执行任务
//         }
//         return;
//     }

} // namespace xb

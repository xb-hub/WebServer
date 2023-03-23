//
// Created by 许斌 on 2021/10/26.
//

#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_
#include <functional>
// #include <pthread.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <list>
#include "Singleton.h"
#include "Thread.h"

namespace xb
{

    class ThreadPool : boost::noncopyable
    {
    public:
        using Task = std::function<void()>; // 任务类型
        using ptr = std::shared_ptr<ThreadPool>;

    public:
        ThreadPool(const std::string &name = "");

    protected:
        const int TASK_NUM; // 任务队列任务数

    public:
        ~ThreadPool();

        bool is_running;
        void AddTask(const Task &task);
        Task TakeTask();
        size_t getSize();
        void start(int thread_num = 8);
        void stop();

    private:
        // 互斥锁和条件变量
        MutexLock mutex_;
        Condition take_cond_, add_cond_;

        std::vector<std::unique_ptr<Thread>> thread_pool; // 存储线程，充当线程池
        std::list<Task> task_queue;                       // 任务队列
        void threadfun();                                 // 线程函数

        const std::string name_;
    };

}

#endif //_THREADPOOL_H_

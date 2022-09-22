//
// Created by 许斌 on 2021/10/26.
//

#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_
#include <functional>
//#include <pthread.h>
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
    typedef std::function<void()> Task; // 任务类型
    typedef std::shared_ptr<ThreadPool> ptr;

public:
    ThreadPool(const std::string& name);

protected:
    const int TASK_NUM;             // 任务队列任务数

public:
    ~ThreadPool();

    bool is_running;
    void AddTask(const Task& task);
    Task TakeTask();
    size_t getSize();
    void start(int thread_num);
    void stop();

private:
    // 互斥锁和条件变量
    MutexLock mutex_;
    Condition cond_;

    std::vector<std::unique_ptr<Thread>> thread_pool;             // 存储线程，充当线程池
    std::list<Task> task_queue;         // 任务队列
    static void* threadfun(void* arg);  // 线程函数

    const std::string name_;
};

}

#endif //_THREADPOOL_H_

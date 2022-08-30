//
// Created by 许斌 on 2021/10/26.
//

#ifndef THREADPOOL_THREADPOOL_H
#define THREADPOOL_THREADPOOL_H
#include <functional>
//#include <pthread.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <list>
#include "Singleton.h"

namespace xb
{

class ThreadPool
{
public:
    typedef std::function<void()> Task; // 任务类型
private:
    // 互斥锁和条件变量
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;

    std::vector<pthread_t> thread_pool; // 存储线程，充当线程池
    std::list<Task> task_queue;         // 任务队列
    static void* threadfun(void* arg);  // 线程函数

public:
    ThreadPool();
    ThreadPool(const int thread_num, const int task_num);

protected:
    const int MAX_THREAD_NUM;           // 线程池最大线程数
    const int MAX_TASK_NUM;             // 任务队列最大任务数

public:
    ~ThreadPool();
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    bool is_running;
    void AddTask(const Task& task);
    Task TakeTask();
    size_t getSize();
    void stop();
};

typedef xb::SingletonPtr<ThreadPool> ThreadPoolMgr;

}

#endif //THREADPOOL_THREADPOOL_H

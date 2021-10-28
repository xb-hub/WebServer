//
// Created by 许斌 on 2021/10/26.
//

#ifndef THREADPOOL_THREADPOOL_H
#define THREADPOOL_THREADPOOL_H
#include <functional>
#include <pthread.h>
#include <vector>
#include <list>

namespace xb {
enum QueueStatus
{
    Full = -1,
    Norm = 1
};

class ThreadPool {
public:
    typedef std::function<void()> Task;
private:
    static ThreadPool* instance;

    pthread_mutex_t pool_lock;
    pthread_cond_t pool_cond;
    static pthread_mutex_t singleton_lock;

    std::vector<pthread_t> thread_pool;
    std::list<Task> task_queue;
    static void* threadfun(void* arg);

private:
    ThreadPool(const int thread_num, const int task_num);
    ~ThreadPool();
    ThreadPool(const ThreadPool&);
    ThreadPool& operator=(const ThreadPool&);

protected:
    const int MAX_THREAD_NUM;
    const int MAX_TASK_NUM;

public:
    bool is_running;
    static ThreadPool* getInstance(const int thread_num,const int task_num);
    int AddTask(Task task);
    Task TakeTask();
    int getSize();
};

}

#endif //THREADPOOL_THREADPOOL_H

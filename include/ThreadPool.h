//
// Created by 许斌 on 2021/10/26.
//

#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_
#include <functional>
#include <latch>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <list>
#include "Singleton.h"
#include "IOEventLoop.h"

namespace xb
{

    class ThreadPool : boost::noncopyable
    {
    public:
        using Task = std::function<void()>; // 任务类型
        using ptr = std::shared_ptr<ThreadPool>;

        ThreadPool();
        ~ThreadPool();

        bool is_running;
        void start();
        void stop();

        IOEventLoop::ptr getOneLoopFromPool();

    private:
        const int m_thread_num;
        std::latch m_latch;
        std::vector<std::jthread> m_thread_pool; // 存储线程，充当线程池
        std::vector<IOEventLoop::ptr> m_event_pool;
        std::atomic_int32_t m_loop_index;
    };

}

#endif //_THREADPOOL_H_

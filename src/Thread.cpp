//
// Created by 许斌 on 2022/9/15.
//

#include "Thread.h"
#include "Log.h"
using namespace xb;

Thread::Thread(const std::string name, std::function<void()> cb) :
        m_name(name),
        m_cb(cb)
{
    int ret = pthread_create(&m_thread, nullptr, Thread::run, this);
    if(ret < 0)
    {
        LOG_DEBUG("Thread creat failure!");
    }
}

void Thread::join()
{
    pthread_join(m_thread, nullptr);
}


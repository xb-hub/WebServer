//
// Created by 许斌 on 2022/9/15.
//

#include "Thread.h"
#include "Log.h"

namespace xb
{

// static Logger::ptr logger = GET_ROOT_LOGGER();

static thread_local std::string thread_name = "UNKNOWN";

ThreadData::ThreadData(const std::string& name, ThreadFunc func, pid_t* tid, CountDownLatch* latch) :
        name_(name),
        func_(std::move(func)),
        tid_(tid),
        latch_(latch)
{}

ThreadData::~ThreadData()
{
    if(tid_ != nullptr) delete tid_;
    if(latch_ != nullptr) delete latch_;
}

void ThreadData::runInThread()
{
    *tid_ = GetThreadId();
    tid_ = nullptr;
    latch_->countDown();
    latch_ = nullptr;
    thread_name = name_.empty() ? "unknownThread" : name_.c_str();
    pthread_setname_np(pthread_self(), name_.substr(0, 15).c_str());
    try
    {
        func_();
    }
    catch(const std::exception& e)
    {
        LOG_FATAL(GET_ROOT_LOGGER(), "run Thread failure!");
    }
    
}

Thread::Thread(const std::string& name, std::function<void()> func) :
        name_(name),
        func_(std::move(func)),
        started_(false),
        join_(false),
        threadId_(0),
        tid_(0),
        latch_(1)
{}

Thread::~Thread()
{
    if(started_ && !join_)
    {
        pthread_detach(threadId_);
    }
}

void Thread::join()
{
    if(started_ && !join_)
    {
        join_ = true;
        pthread_join(threadId_, nullptr);
    }
}

void Thread::start()
{
    assert(!started_);
    started_ = true;
    ThreadData* data = new ThreadData(name_, func_, &tid_, &latch_);
    if(pthread_create(&threadId_, nullptr, Thread::run, data))
    {
        started_ = false;
        delete data;
        // LOG_DEBUG(GET_ROOT_LOGGER(), "Thread creat failure!");
        throw std::logic_error("pthread_creat error");
    }
    else
    {
        latch_.wait();  // 等待创建线程开始运行，也可以使用信号量实现。
        assert(tid_ > 0);
    }
}

void* Thread::run(void* arg)
{
    ThreadData* data = static_cast<ThreadData*>(arg);
    data->runInThread();
    delete data;
    return nullptr;
}

} // namespace xb

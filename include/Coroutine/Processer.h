#ifndef _PROCESSER_H_
#define _PROCESSER_H_

// #include <condition_variable>
#include <list>

#include "Coroutine/Coroutine.h"
#include "Coroutine/Scheduler.h"

namespace xb
{

class Processer
{

public:
    using ptr = std::shared_ptr<Processer>;

    Processer(/* args */);
    ~Processer();

private:
    Scheduler::ptr scheduler_;
    int threadId_;

    volatile bool active_ = true;

    Coroutine::ptr runCoroutine_;
    Coroutine::ptr nextCoroutine_;

    std::list<Coroutine::ptr> runQueue_;
    std::list<Coroutine::ptr> waitQueue_;

public:
    static Processer::ptr & GetCurrentProcesser();
    static Scheduler::ptr getCurrentScheduler();

};

} // namespace xb


#endif
#include "Processer.h"

namespace xb
{
    Processer::Processer(Scheduler::ptr scheduler, int id)
        : scheduler_(scheduler),
          processer_id_(id)
    {}

    void Processer::process()
    {
        GetCurrentProcesser() = this;
        while(!scheduler_->CanStop())
        {
            
        }
    }

    Processer* & Processer::GetCurrentProcesser()
    {
        static thread_local Processer* proc = nullptr;
        return proc;
    }

    Scheduler::ptr Processer::GetCurrentScheduler()
    {
        auto proc = GetCurrentProcesser();
        return proc ? proc->scheduler_ : nullptr;
    }

    void Processer::addTask(Func& func)
    {
        addTask(Coroutine(func));
    }

    void Processer::addTask(Coroutine&& routine)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        new_list_.emplace_back(std::forward<Coroutine>(routine));
        if(waiting_)    cond_.notify_all();
        else    notified_ = true;
    }



} // namespace xb

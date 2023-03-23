#ifndef _PROCESSER_H_
#define _PROCESSER_H_

#include <stdint.h>
#include <list>
#include <functional>
#include <condition_variable>
#include "Thread.h"
#include "Coroutine.h"
#include "Scheduler.h"

namespace xb
{
    class Processer
    {
    
    public:
        using Func = std::function<void()>;
        ~Processer();

        static Processer* & GetCurrentProcesser();
        static Scheduler::ptr GetCurrentScheduler();
        static Coroutine::ptr GetCurrentRoutine();
        static bool IsRoutine();

        static void Yield();
        static void Yield2Hold();

        Scheduler::ptr GetScheduler() const { return scheduler_; }

    private:
        explicit Processer(Scheduler::ptr scheduler, int id);

        void addNewTask();
        void addTask(Func& func);
        void addTask(Coroutine&& routine);

        bool isBlocking();
        bool isWaiting() const { return waiting_; }

        void process();

    private:
        Scheduler::ptr scheduler_;

        int processer_id_;

        volatile bool active_ = true;
        volatile uint64_t switch_count_;

        Coroutine* current_routine_;
        Coroutine* next_routine_;

        std::list<Coroutine::ptr> running_list_;
        std::list<Coroutine::ptr> wait_list_;
        std::list<Coroutine::ptr> new_list_;

        std::condition_variable_any cond_;
        std::atomic_bool waiting_{false};
        bool notified_ = false;

        std::mutex mutex_;

    };
    
} // namespace xb


#endif
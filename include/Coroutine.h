#ifndef _COROUTINE_H_
#define _COROUTINE_H_

#include <memory>
#include <functional>
#include <ucontext.h>
#include <boost/noncopyable.hpp>
#include <atomic>
#include "Log.h"

namespace xb
{

    class Coroutine : public std::enable_shared_from_this<Coroutine>
    {
    public:
        friend class Scheduler;
        using ptr = std::shared_ptr<Coroutine>;
        using CoroutineFunc = std::function<void()>;

        enum CoroutineState
        {
            INIT,
            HOLD,
            EXEC,
            TERM,
            READY,
            EXCEPTION
        };

    private:
        Coroutine();
        void setThis(Coroutine *coroutine);

    public:
        explicit Coroutine(CoroutineFunc func, bool use_caller = false, uint64_t stack_size = 0);
        ~Coroutine();

        // CoroutineState getState() { return state_; }
        uint64_t getId() { return coroutineId_; }

        // 重置协程函数
        void reset(CoroutineFunc func);
        // 协程切换到后台
        void swapIn();
        void swapOut();
        void call();
        void back();

        static Coroutine::ptr GetThis();
        static void Yield();
        static void Yield2Hold();

        static void MainFunc();
        static void CallMainFunc();

        bool finish();

    private:
        uint64_t coroutineId_; // 协程ID
        uint64_t stack_size_;  // 栈大小
        ucontext_t context_;   // 上下文
        CoroutineFunc func_;   // 执行函数
        CoroutineState state_; // 协程状态
        void *stack_;          // 协程栈
        // Processer::ptr proc_;       // 协程管理器

        static thread_local Coroutine *current_coroutine;
        static thread_local Coroutine::ptr pre_coroutine_ptr;
        static std::atomic_uint32_t CoroutineNum_;
    };

} // namespace xb

#endif //_COROUTINE_H_
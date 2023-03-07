#include "Coroutine.h"
#include "Log.h"
#include "Scheduler.h"
#include "util.h"

namespace xb
{

    thread_local Coroutine *Coroutine::current_coroutine = nullptr;
    thread_local Coroutine::ptr Coroutine::pre_coroutine_ptr{};
    std::atomic_uint32_t Coroutine::CoroutineNum_{0};

    class StackAllocator
    {
    public:
        static void *Alloc(uint64_t size)
        {
            return ::malloc(size);
        }

        static void Dealloc(void *p, uint64_t size)
        {
            free(p);
        }
    };

    Coroutine::Coroutine() : stack_(nullptr),
                             state_(EXEC),
                             func_(),
                             stack_size_(0),
                             context_(),
                             coroutineId_(0)
    {
        //    std::cout << "主协程构造函数" << std::endl;
        setThis(this);
        if (getcontext(&context_))
        {
            LOG_ERROR(GET_ROOT_LOGGER(), "get context failure!");
        }
        ++CoroutineNum_;
        coroutineId_ = CoroutineNum_;
        // LOG_FMT_DEBUG(stdout_logger, "创建主协程 id=%d, total=%d", static_cast<int>(coroutineId_), static_cast<int>(CoroutineNum_));
    }

    Coroutine::Coroutine(CoroutineFunc func, bool use_caller, uint64_t stack_size) : func_(std::move(func)),
                                                                                     stack_size_(100 * 1024),
                                                                                     state_(INIT),
                                                                                     stack_(nullptr),
                                                                                     context_()
    {
        //    std::cout << "子协程构造函数" << std::endl;
        setThis(this);
        if (getcontext(&context_))
        {
            LOG_ERROR(GET_ROOT_LOGGER(), "get context failure!");
        }

        stack_ = StackAllocator::Alloc(stack_size_);
        context_.uc_link = nullptr;
        context_.uc_stack.ss_sp = stack_;
        context_.uc_stack.ss_size = stack_size_;

        if (use_caller)
        {
            makecontext(&context_, &Coroutine::CallMainFunc, 0);
        }
        else
        {
            makecontext(&context_, &Coroutine::MainFunc, 0);
        }
        ++CoroutineNum_;
        coroutineId_ = CoroutineNum_;
        // LOG_FMT_DEBUG(stdout_logger, "创建子协程 id=%d, total=%d", static_cast<int>(coroutineId_), static_cast<int>(CoroutineNum_));
    }

    Coroutine::~Coroutine()
    {
        if (stack_)
        {
            //    std::cout << "子协程析构函数" << std::endl;
            assert(state_ == INIT || state_ == TERM || state_ == EXCEPTION);
            StackAllocator::Dealloc(stack_, stack_size_);
        }
        else
        {
            //    std::cout << "主协程析构函数" << std::endl;
            assert(!func_ && state_ == EXEC);
            if (Coroutine::current_coroutine == this)
            {
                setThis(nullptr);
            }
        }
        --CoroutineNum_;
        // LOG_FMT_DEBUG(stdout_logger, "销毁协程 id=%d, total=%d", static_cast<int>(coroutineId_), static_cast<int>(CoroutineNum_));
    }

    void Coroutine::setThis(Coroutine *coroutine)
    {
        current_coroutine = coroutine;
    }

    Coroutine::ptr Coroutine::GetThis()
    {
        if (Coroutine::current_coroutine != nullptr)
        {
            // 利用shared_from_this获取this的智能指针
            return Coroutine::current_coroutine->shared_from_this();
        }
        // 创建主协程
        Coroutine::pre_coroutine_ptr.reset(new Coroutine());
        return Coroutine::current_coroutine->shared_from_this();
    }

    void Coroutine::reset(CoroutineFunc func)
    {
        assert(stack_);
        assert(state_ == INIT || state_ == TERM || state_ == EXCEPTION);
        func_ = std::move(func);
        if (getcontext(&context_))
        {
            LOG_ERROR(GET_ROOT_LOGGER(), "get context failure!");
        }
        context_.uc_link = nullptr;
        context_.uc_stack.ss_sp = stack_;
        context_.uc_stack.ss_size = stack_size_;

        makecontext(&context_, &Coroutine::MainFunc, 0);
        state_ = INIT;
    }

    void Coroutine::call()
    {
        // 主协程切换到子协程
        assert(state_ != EXEC);
        setThis(this);
        state_ = EXEC;
        LOG_FMT_DEBUG(stdout_logger, "call pre_coroutine_ptr: [%d]  %d->%d", GetThreadId(), Coroutine::pre_coroutine_ptr->coroutineId_, coroutineId_);
        // 当前线程的主协程切换到子协程，保存当前上下文到主协程
        if (swapcontext(&Coroutine::pre_coroutine_ptr->context_, &context_))
        {
            LOG_ERROR(GET_ROOT_LOGGER(), "swapIn failure!");
        }
    }

    void Coroutine::back()
    {
        // 子协程切换到主协程
        assert(stack_);
        setThis(Coroutine::pre_coroutine_ptr.get());
        LOG_FMT_DEBUG(stdout_logger, "back pre_coroutine_ptr: [%d]  %d->%d", GetThreadId(), coroutineId_, Coroutine::pre_coroutine_ptr->coroutineId_);
        // 当前线程的子协程切换到主协程，保存当前上下文到子协程;
        if (swapcontext(&context_, &Coroutine::pre_coroutine_ptr->context_))
        {
            LOG_ERROR(GET_ROOT_LOGGER(), "swapOut failure!");
        }
    }

    void Coroutine::swapIn()
    {
        // 主协程切换到子协程
        assert(state_ != EXEC);
        setThis(this);
        state_ = EXEC;
        LOG_FMT_DEBUG(stdout_logger, "swapIn RootRoutineID: [%d]  %d->%d", GetThreadId(), Scheduler::GetRootRoutine()->coroutineId_, coroutineId_);
        // 主线程的主协程切换到当前协程，保存当前上下文到主协程
        if (swapcontext(&(Scheduler::GetRootRoutine()->context_), &context_))
        {
            LOG_ERROR(GET_ROOT_LOGGER(), "swapIn failure!");
        }
    }

    void Coroutine::swapOut()
    {
        // 子协程切换到主协程
        assert(stack_);
        setThis(Scheduler::GetRootRoutine());
        LOG_FMT_DEBUG(stdout_logger, "swapOut RootRoutineID: [%d]  %d->%d", GetThreadId(), coroutineId_, Scheduler::GetRootRoutine()->coroutineId_);
        // 当前协程切换到主线程主协程，保存当前上下文到子协程;
        if (swapcontext(&context_, &(Scheduler::GetRootRoutine()->context_)))
        {
            LOG_ERROR(GET_ROOT_LOGGER(), "swapOut failure!");
        }
    }

    void Coroutine::Yield2Hold()
    {
        Coroutine::ptr cur = GetThis();
        cur->state_ = HOLD;
        cur->swapOut();
    }

    void Coroutine::Yield()
    {
        Coroutine::ptr cur = GetThis();
        cur->state_ = READY;
        cur->swapOut();
    }

    void Coroutine::MainFunc()
    {
        // std::cout << "routine start" << std::endl;
        Coroutine::ptr cur = GetThis();
        try
        {
            cur->func_();
            cur->func_ = nullptr;
            cur->state_ = TERM;
            // LOG_INFO(stdout_logger, "idle fiber term");
        }
        catch (const std::exception &e)
        {
            cur->state_ = EXCEPTION;
            LOG_ERROR(stdout_logger, "exec function failure!");
        }
        // 释放share_ptr引用计数，否则cur永远无法释放。
        Coroutine *ptr = cur.get();
        cur.reset();
        // LOG_INFO(stdout_logger, "swapOut Main");
        ptr->swapOut();

        assert(false && "finish");
    }

    void Coroutine::CallMainFunc()
    {
        Coroutine::ptr cur = GetThis();
        try
        {
            cur->func_();
            cur->func_ = nullptr;
            cur->state_ = TERM;
        }
        catch (const std::exception &e)
        {
            cur->state_ = EXCEPTION;
            LOG_ERROR(stdout_logger, "exec function failure!");
        }
        // 释放share_ptr引用计数，否则cur永远无法释放。
        Coroutine *ptr = cur.get();
        cur.reset();
        ptr->back();

        assert(false && "finish");
    }

    bool Coroutine::finish()
    {
        return (state_ == TERM || state_ == EXCEPTION);
    }

} // namespace xb

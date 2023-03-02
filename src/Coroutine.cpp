#include "Coroutine.h"
#include "Log.h"
#include "Scheduler.h"
#include <opencv2/opencv.hpp>

namespace xb
{

class StackAllocator
{
public:
    static void* Alloc(uint64_t size)
    {
        return ::malloc(size);
    }

    static void Dealloc(void* p, uint64_t size)
    {
        free(p);
    }
};

thread_local Coroutine* Coroutine::current_coroutine = nullptr;
thread_local Coroutine::ptr Coroutine::main_coroutine_ptr{};
std::atomic_uint32_t Coroutine::CoroutineID_{0};

Coroutine::Coroutine():
        stack_(nullptr),
        state_(EXEC),
        func_(),
        stack_size_(0),
        context_(),
        coroutineId_(0)
{
//    std::cout << "主协程构造函数" << std::endl;
    setThis(this);
    if(getcontext(&context_))
    {
            LOG_ERROR(GET_ROOT_LOGGER(),"get context failure!");
    }
    coroutineId_++;
}


Coroutine::Coroutine(CoroutineFunc func, uint64_t stack_size) :
        func_(std::move(func)),
        stack_size_(1024*1024),
        state_(INIT),
        stack_(nullptr),
        context_()
{
//    std::cout << "子协程构造函数" << std::endl;
    setThis(this);
    if(getcontext(&context_))
    {
            LOG_ERROR(GET_ROOT_LOGGER(),"get context failure!");
    }

    stack_ = StackAllocator::Alloc(stack_size_);
    context_.uc_link = nullptr;
    context_.uc_stack.ss_sp = stack_;
    context_.uc_stack.ss_size = stack_size_;

    makecontext(&context_, &Coroutine::MainFunc, 0);
    coroutineId_++;
}

Coroutine::~Coroutine()
{
    if(stack_)
    {
    //    std::cout << "子协程析构函数" << std::endl;
        assert(state_ == INIT || state_ == TERM || state_ == EXCEPTION);
        StackAllocator::Dealloc(stack_, stack_size_);
    }
    else
    {
    //    std::cout << "主协程析构函数" << std::endl;
        assert(!func_ && state_ == EXEC);
        if(Coroutine::current_coroutine == this)
        {
            setThis(nullptr);
        }
    }
}

void Coroutine::setThis(Coroutine* coroutine)
{
    current_coroutine = coroutine;
}

Coroutine::ptr Coroutine::GetThis()
{
    if(Coroutine::current_coroutine != nullptr)
    {
        // 利用shared_from_this获取this的智能指针
        return Coroutine::current_coroutine->shared_from_this();
    }
    // 创建主协程
    Coroutine::main_coroutine_ptr.reset(new Coroutine());
    return Coroutine::main_coroutine_ptr->shared_from_this();
}

void Coroutine::reset(CoroutineFunc func)
{
    assert(stack_);
    assert(state_ == INIT || state_ == TERM || state_ == EXCEPTION);
    func_ = std::move(func);
    if(getcontext(&context_))
    {
            LOG_ERROR(GET_ROOT_LOGGER(),"get context failure!");
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
    assert(state_ == INIT || state_ == READY || state_ == HOLD);
    setThis(this);
    state_ = EXEC;
    // 当前线程的主协程切换到子协程，保存当前上下文到主协程
    if(swapcontext(&Coroutine::main_coroutine_ptr->context_, &context_))
    {
        LOG_ERROR(GET_ROOT_LOGGER(), "swapIn failure!");
    }
}

void Coroutine::back()
{
    // 子协程切换到主协程
    assert(stack_);
    setThis(Coroutine::main_coroutine_ptr.get());
    // 当前线程的子协程切换到主协程，保存当前上下文到子协程;
    if(swapcontext(&context_, &Coroutine::main_coroutine_ptr->context_))
    {
        LOG_ERROR(GET_ROOT_LOGGER(), "swapOut failure!");
    }
}

void Coroutine::swapIn()
{
    // 主协程切换到子协程
    assert(state_ == INIT || state_ == READY || state_ == HOLD);
    setThis(this);
    state_ = EXEC;
    // 主线程的主协程切换到当前协程，保存当前上下文到主协程
    if(swapcontext(&(Scheduler::GetRootRoutine()->context_), &context_))
    {
        LOG_ERROR(GET_ROOT_LOGGER(), "swapIn failure!");
    }
}

void Coroutine::swapOut()
{
    // 子协程切换到主协程
    assert(stack_);
    setThis(Scheduler::GetRootRoutine());
    // 当前协程切换到主线程主协程，保存当前上下文到子协程;
    if(swapcontext(&context_, &(Scheduler::GetRootRoutine()->context_)))
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
    cur->state_ = HOLD;
    cur->back();
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
    }
    catch(std::exception& e)
    {
        cur->state_ = EXCEPTION;
        LOG_ERROR(GET_ROOT_LOGGER(), "exec function failure!");
    }
    // 释放share_ptr引用计数，否则cur永远无法释放。
    Coroutine* ptr = cur.get();
    cur.reset();
    if (Scheduler::GetThis() &&
        Scheduler::GetThis()->getRootThreadId() == GetThreadId() &&
        Scheduler::GetThis()->GetRootRoutine() != ptr)
    { // 调度器实例化时 use_caller 为 true, 并且当前协程所在的线程就是 root thread
        // 如果是主线程，将切换到主线程中的主协程
        ptr->swapOut();
    }
    else
    {
        // 如果是线程池中的线程，切换到当前线程中的主协程
        ptr->back();
    }
    ptr->back();
    // ptr->swapOut();
    assert(false && "finish");
}

bool Coroutine::finish()
{
    return (state_ == TERM || state_ == EXCEPTION);
}

} // namespace xb

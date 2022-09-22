#ifndef _COROUTINE_H_
#define _COROUTINE_H_

#include <memory>
#include <functional>
#include <sys/ucontext.h>
#include <boost/noncopyable.hpp>

#define STACK_SIZE (1024*1024)

namespace xb
{

class Schedule
{
public:
    Schedule(/* args */);
    ~Schedule();

private:
    static const int DEFAULT_COROUTINE = 16;    // 默认协程数量
    char stack[STACK_SIZE];
    ucontext_t main_;                            // 当前协程上下文
    int num_;        // 当前协程数量

};
    
class Coroutine : boost::noncopyable
{
public:
    typedef std::shared_ptr<Coroutine> ptr;
    typedef std::function<void()> CoroutineFunc;

    enum CoroutineState
    {
        READY,
        RUNNING,
        SUSPEND,
        DEAD
    };

public:
    Coroutine(CoroutineFunc func, uint64_t stack_size);
    ~Coroutine();

    static Coroutine::ptr GetThis();
    static void Yield2Ready();
    static void Yield2Suspend();

private:
    uint64_t coroutineId_;      // 协程ID
    uint64_t stack_size_;       // 栈大小
    ucontext_t context_;        // 上下文
    CoroutineFunc func_;        // 执行函数
    CoroutineState state_;      // 协程状态

    friend class Schedule;
};
    
} // namespace xb

#endif //_COROUTINE_H_
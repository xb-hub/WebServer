#ifndef _SCHEDULE_H_
#define _SCHEDULE_H_
#include <vector>
#include <list>


#include "Coroutine/Thread.h"
#include "Log/Log.h"
#include "Coroutine/Coroutine.h"

namespace xb
{

class Scheduler
{
public:
    using ptr = std::shared_ptr<Scheduler>;
    using TaskFunc = std::function<void()>;

    // 线程执行协程
    struct Task
    {
        using ptr = std::shared_ptr<Task>;
        
        Coroutine::ptr routine_;
        TaskFunc func_;
        u_int64_t threadID_;

        Task() :
            threadID_(-1)
        {}

        Task(Coroutine::ptr routine, long tid) :
            routine_(std::move(routine)),
            threadID_(tid)
        {}

        Task(const TaskFunc& func, long tid) :
            func_(func),
            threadID_(tid)
        {}

        Task(TaskFunc&& func, long tid) :
            func_(std::move(func)),
            threadID_(tid)
        {}

        Task(Task& rhs) = default;
        Task& operator=(const Task& rhs) = default;

        void reset()
        {
            routine_ = nullptr;
            func_ = nullptr;
            threadID_ = -1;
        }

    };

public:
    friend class Fiber;

    Scheduler(size_t thread_num, bool usecall, const std::string name = "");
    virtual ~Scheduler();

    void start();
    void stop();
    void run();

    pid_t getRootThreadId();

    static Coroutine* GetRootRoutine();
    static Scheduler* GetThis();

    template<typename Executable>
    void addTask(Executable&& callback, pid_t threadID = -1, bool prior = false)
    {
        MutexLockGuard lock(mutex_);
        Task::ptr task = std::make_shared<Task>(std::forward<Executable>(callback), threadID);
        if(task->routine_ || task->func_)
        {
            if(prior)   task_list_.push_front(task);
            else        task_list_.push_back(task);   
        }
    }

protected:
    virtual void onIdel();
    virtual bool isStop();
    virtual void tick();

private:
    Task* takeTask();
    
private:
    MutexLock mutex_;
    Condition take_cond_, add_cond_;

    size_t exec_thread_num_;

    const std::string name_;
    pid_t root_threadID_;

    std::vector<Thread::ptr> thread_list_;
    std::list<Task::ptr> task_list_;

    size_t thread_num_;
    Coroutine::ptr root_routine_;

    bool running_;

    static thread_local Coroutine* current_root_routine_;
    static thread_local Scheduler* current_schedule_;

};

} // namespace xb


#endif
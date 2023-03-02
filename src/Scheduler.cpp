#include <assert.h>
#include "Scheduler.h"
#include "util.h"

namespace xb
{

// static Logger::ptr system_logger = GET_LOGGER("system");
thread_local Coroutine* Scheduler::current_root_routine_ = nullptr;
thread_local Scheduler* Scheduler::current_schedule_ = nullptr;

Scheduler* Scheduler::GetThis()
{
    return current_schedule_;
}

Coroutine* Scheduler::GetRootRoutine()
{
    return current_root_routine_;
}

pid_t Scheduler::getRootThreadId()
{
    return root_threadID_;
}


Scheduler::Scheduler(size_t thread_num, bool usecall, const std::string name) :
    name_(std::move(name)),
    running_(false),
    mutex_(),
    take_cond_(mutex_),
    add_cond_(mutex_),
    exec_thread_num_(0)
{
    if(usecall)
    {
        // 创建主协程
        Coroutine::GetThis();
        assert(GetThis() == nullptr);
        current_schedule_ = this;
        root_routine_ = std::make_shared<Coroutine>(std::bind(&Scheduler::run, this));
        current_root_routine_ = root_routine_.get();
        root_threadID_ = GetThreadId();
    }
    else
    {
        root_threadID_ = -1;
    }
    thread_num_ = thread_num;
}

Scheduler::~Scheduler()
{
    // LOG_DEBUG(system_logger, "调用 Scheduler::~Scheduler()");
    // std::cout << "调用 Scheduler::~Scheduler()" << std::endl;
    if(GetThis() == this)
    {
        current_schedule_ = nullptr;
    }
}

void Scheduler::start()
{
    // LOG_DEBUG(system_logger, "调用 Scheduler::start()");
    // std::cout << "调用 Scheduler::start()" << std::endl;
    if (running_)
    {   // 调度器已经开始工作
        return;
    }
    running_ = true;
    assert(thread_list_.empty());
    for(size_t i = 0; i < thread_num_; i++)
    {
        thread_list_.emplace_back(std::make_shared<Thread>(std::to_string(i), std::bind(&Scheduler::run, this)));
        thread_list_[i]->start();
    }
}

void Scheduler::stop()
{
    // LOG_DEBUG(system_logger, "调用 Scheduler::stop()");
    // std::cout << "调用 Scheduler::stop()" << std::endl;
    if(root_routine_ && thread_num_ == 0 && root_routine_->finish() || root_routine_->getState() == Coroutine::INIT)
    {
        running_ = false;
        if(isStop())    return;
    }
    running_ = false;

    if (root_routine_)
    {
        if (!isStop())
        {
            root_routine_->call();
        }
    }

    { // join 所有子线程
        for (auto& t : thread_list_)
        {
            t->join();
        }
        thread_list_.clear();
    }
    if (isStop())
    {
        return;
    }
}

bool Scheduler::isStop()
{
    MutexLockGuard lock(mutex_);
    return task_list_.empty() && exec_thread_num_ == 0 && !running_;
}

void Scheduler::onIdel()
{
    while(!isStop())
    {
        Coroutine::Yield2Hold();
    }
}

void Scheduler::tick()
{
    
}

Scheduler::Task* Scheduler::takeTask()
{
}

void Scheduler::run()
{
    // LOG_DEBUG(system_logger, "调用 Scheduler::run()");
    // std::cout << "调用 Scheduler::run()" << GetThreadId() << std::endl;
    current_schedule_ = this;
    if(GetThreadId() != root_threadID_)
    {
        // 给每个线程创建主协程
        current_root_routine_ = Coroutine::GetThis().get();
    }
    auto idle_routine = std::make_shared<Coroutine>(std::bind(&Scheduler::onIdel, this));
    Task task;
    while(running_)
    {
        task.reset();
        {
            // 作用域锁
            MutexLockGuard lock(mutex_);
            for(auto it = task_list_.begin(); it != task_list_.end(); ++it)
            {
                // 任务指定特定线程执行，但不是当前线程
                if((*it)->threadID_ != -1 && (*it)->threadID_ != GetThreadId())
                {
                    continue;
                }
                assert((*it)->func_ || (*it)->routine_);
                // 协程正在执行
                if((*it)->routine_ && (*it)->routine_->getState() == Coroutine::EXEC)
                {
                    continue;
                }
                task = **it;
                ++exec_thread_num_;
                task_list_.erase(it);
                break;
            }
        }
        // 如果任务是函数，构造协程
        if(task.func_)
        {
            // std::cout << "执行函数" << std::endl;
            task.routine_ = std::make_shared<Coroutine>(std::move(task.func_));
            task.func_ = nullptr;
        }
        if(task.routine_ && !task.routine_->finish())
        {
            // std::cout << "执行协程" << std::endl;
            // if(GetThreadId() == main_threadID_)
            // {
            //     std::cout << "size: " << task_list_.size() << std::endl;
            //     task.routine_->swapIn();
            //     --exec_thread_num_;
            // }
            task.routine_->swapIn();
            --exec_thread_num_;
            
            Coroutine::CoroutineState state = task.routine_->getState();
            if(state == Coroutine::READY)
            {
                addTask(std::move(task.routine_), task.threadID_);
            }
            else if(state == Coroutine::EXCEPTION)
            {
                task.routine_->state_ = Coroutine::HOLD;
            }
            task.reset();
        }
        else    // 任务队列为空
        {
            // std::cout << "empty" << std::endl;
            if(idle_routine->finish())
            {
                break;
            }
            idle_routine->swapIn();
            if(!idle_routine->finish()) idle_routine->state_ = Coroutine::HOLD;
        }
        // std::cout << "finish" << std::endl;
    }
}

} // namespace xb

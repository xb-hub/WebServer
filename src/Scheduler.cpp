#define LOG
#include <assert.h>
#include "Scheduler.h"
#include "util.h"
#include "Hook.h"

namespace xb
{

    thread_local Coroutine *Scheduler::current_root_routine_ = nullptr;
    thread_local Scheduler *Scheduler::current_schedule_ = nullptr;

    Scheduler *Scheduler::GetThis()
    {
        return current_schedule_;
    }

    void Scheduler::setThis()
    {
        current_schedule_ = this;
    }

    Coroutine *Scheduler::GetRootRoutine()
    {
        return current_root_routine_;
    }

    pid_t Scheduler::getRootThreadId()
    {
        return root_threadID_;
    }

    Scheduler::Scheduler(size_t thread_num, bool usecall, const std::string name) : name_(std::move(name)),
                                                                                    running_(false),
                                                                                    mutex_(),
                                                                                    exec_thread_num_(0)
    {
        if (usecall)
        {
            // 创建主协程
            Coroutine::GetThis();
            --thread_num;
            assert(GetThis() == nullptr);
            current_schedule_ = this;
            root_routine_ = std::make_shared<Coroutine>(std::bind(&Scheduler::run, this), true, 0);
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
#ifdef LOG
        LOG_DEBUG(stdout_logger, "调用 Scheduler::~Scheduler()");
#endif
        if (GetThis() == this)
        {
            current_schedule_ = nullptr;
        }
    }

    void Scheduler::start()
    {
#ifdef LOG
        LOG_DEBUG(stdout_logger, "调用 Scheduler::start()");
#endif
        if (running_)
        { // 调度器已经开始工作
            return;
        }
        running_ = true;
        assert(thread_list_.empty());
        for (size_t i = 0; i < thread_num_; i++)
        {
            thread_list_.emplace_back(std::make_shared<Thread>(std::to_string(i), std::bind(&Scheduler::run, this)));
            thread_list_[i]->start();
        }
    }

    void Scheduler::stop()
    {
#ifdef LOG
        LOG_DEBUG(stdout_logger, "调用 Scheduler::stop()");
#endif
        if (root_routine_ && thread_num_ == 0 && (root_routine_->finish() || root_routine_->state_ == Coroutine::INIT))
        {
            running_ = false;
            if (CanStop())
                return;
        }
        if (root_threadID_ != -1)
            assert(GetThis() == this);
        else
            assert(GetThis() != this);

        running_ = false;
        for (size_t i = 0; i < thread_num_; ++i)
        {
            tickle(); // 类似于信号量，唤醒
        }

        if (root_routine_)
        {
            tickle();
        }

        if (root_routine_)
        {
            if (!CanStop())
            {
                root_routine_->call();
            }
        }

        std::vector<Thread::ptr> thrs;
        {
            MutexLockGuard lock(mutex_);
            thrs.swap(thread_list_);
        }

        for (auto &t : thrs)
        {
            t->join();
        }
    }

    bool Scheduler::CanStop()
    {
        MutexLockGuard lock(mutex_);
        return task_list_.empty() && exec_thread_num_ == 0 && !running_;
    }

    void Scheduler::onIdel()
    {
#ifdef LOG
        LOG_DEBUG(stdout_logger, "调用 Scheduler::onIdel()");
#endif
        while (!CanStop())
        {
            Coroutine::Yield2Hold();
        }
    }

    void Scheduler::tickle()
    {
#ifdef LOG
        LOG_DEBUG(stdout_logger, "调用 Scheduler::tickle()");
#endif
    }

    void Scheduler::run()
    {
#ifdef LOG
        LOG_DEBUG(stdout_logger, "调用 Scheduler::run()");
#endif
        setThis();
        set_hook_enable(true);
        if (GetThreadId() != root_threadID_)
        {
            // 给每个线程创建主协程
            current_root_routine_ = Coroutine::GetThis().get();
        }
        auto idle_routine = std::make_shared<Coroutine>(std::bind(&Scheduler::onIdel, this));
        Task task;
        while (true)
        {
            task.reset();
            bool is_active = false;
            bool is_tickle = false;
            {
                // 作用域锁
                MutexLockGuard lock(mutex_);
                auto it = task_list_.begin();
                while (it != task_list_.end())
                {
                    // 任务指定特定线程执行，但不是当前线程
                    if ((*it)->threadID_ != -1 && (*it)->threadID_ != GetThreadId())
                    {
                        ++it;
                        is_tickle = true;
                        continue;
                    }
                    assert((*it)->func_ || (*it)->routine_);
                    // 协程正在执行
                    if ((*it)->routine_ && (*it)->routine_->state_ == Coroutine::EXEC)
                    {
                        ++it;
                        continue;
                    }
                    task = **it;
                    ++exec_thread_num_;
                    task_list_.erase(it);
                    is_active = true;
                    break;
                }
                is_tickle |= it != task_list_.end();
            }
            if (is_tickle)
            {
                tickle();
            }
            // 如果任务是函数，构造协程
            if (task.func_)
            {
                // std::cout << "执行函数" << std::endl;
                task.routine_ = std::make_shared<Coroutine>(std::move(task.func_));
                task.func_ = nullptr;
            }
            if (task.routine_ && !task.routine_->finish())
            {
                // LOG_INFO(stdout_logger, "执行协程");
                // std::cout <<  << std::endl;
                // if(GetThreadId() == main_threadID_)
                // {
                //     std::cout << "size: " << task_list_.size() << std::endl;
                //     task.routine_->swapIn();
                //     --exec_thread_num_;
                // }
                task.routine_->swapIn();
                --exec_thread_num_;

                Coroutine::CoroutineState state = task.routine_->state_;
                if (state == Coroutine::READY)
                {
                    addTask(std::move(task.routine_), task.threadID_);
                }
                else if (!task.routine_->finish())
                {
                    task.routine_->state_ = Coroutine::HOLD;
                }
                task.reset();
            }
            else // 任务队列为空
            {
                // LOG_FMT_INFO(stdout_logger, "task_list: %d,  exec_thread_num_: %d,  running_: %d, state: %d", task_list_.size(), exec_thread_num_, (int)running_, (int)idle_routine->state_);
                if (is_active)
                {
                    // LOG_INFO(stdout_logger, "idle fiber activate");
                    --exec_thread_num_;
                    continue;
                }
                if (idle_routine->state_ == Coroutine::TERM)
                {
                    break;
                }
                ++idle_thread_num_;
                idle_routine->swapIn();
                // LOG_FMT_INFO(stdout_logger, "state: %d",(int)idle_routine->state_);
                --idle_thread_num_;
                if (!idle_routine->finish())
                {
                    // LOG_INFO(stdout_logger, "idle fiber HOLD");
                    idle_routine->state_ = Coroutine::HOLD;
                }
            }
        }
    }

} // namespace xb

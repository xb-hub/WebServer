#ifndef _SCHEDULE_H_
#define _SCHEDULE_H_
// #define LOG
#include <vector>
#include <list>

#include "Log.h"
#include "Coroutine.h"
#include "Epoll.h"
#include <thread>
#include <mutex>

namespace xb
{
    static xb::Logger::ptr stdout_logger = GET_ROOT_LOGGER();

    class Scheduler
    {
    public:
        using ptr = std::shared_ptr<Scheduler>;
        using unptr = std::unique_ptr<Scheduler>;
        using TaskFunc = std::function<void()>;

        // 线程执行协程
        struct Task
        {
            using ptr = std::shared_ptr<Task>;

            Coroutine::ptr routine_;
            TaskFunc func_;

            Task()
            {
            }

            Task(Coroutine::ptr routine)
                : routine_(std::move(routine))
            {
            }

            Task(const TaskFunc &func)
                : func_(func)
            {
            }

            Task(TaskFunc &&func)
                : func_(std::move(func))
            {
            }

            Task(Task &rhs) = default;
            Task &operator=(const Task &rhs) = default;

            void reset()
            {
                routine_ = nullptr;
                func_ = nullptr;
            }
        };

    public:
        friend class Coroutine;

        Scheduler(size_t thread_num = 16, bool usecall = true, const std::string name = "");
        virtual ~Scheduler();

        void start();
        void stop();
        void run();

        pid_t getRootThreadId();
        const std::string &getName() const { return name_; }

        static Coroutine *GetRootRoutine();
        static Scheduler *GetThis();
        void setThis();

        template <typename Executable>
        void addTask(Executable&& callback)
        {
#ifdef LOG
            LOG_INFO(stdout_logger, "调用 Scheduler::AddTask()");
#endif
            bool is_tickle = false;
            {
                std::scoped_lock<std::mutex> lock(mutex_);
                Task::ptr task = std::make_shared<Task>(std::forward<Executable>(callback));
                is_tickle = task_list_.empty();
                if (task->routine_ || task->func_)
                {
                    task_list_.push_back(task);
                }
            }
            if (is_tickle)
                tickle();
        }

        template <typename InputIterator>
        void addTask(InputIterator begin, InputIterator end)
        {
#ifdef LOG
            LOG_INFO(stdout_logger, "调用 Scheduler::AddTask()");
#endif
            bool is_tickle = false;
            {
                std::scoped_lock<std::mutex> lock(mutex_);
                while (begin != end)
                {
                    Task::ptr task = std::make_shared<Task>(*begin);
                    is_tickle = task_list_.empty() || is_tickle;
                    if (task->routine_ || task->func_)
                    {
                        task_list_.push_back(task);
                    }
                    ++begin;
                }
            }
            if (is_tickle)
                tickle();
        }

    protected:
        virtual void onIdel();
        virtual bool CanStop();
        virtual void tickle();

        bool hasIdleThread() { return idle_thread_num_ > 0; }

    private:
        std::string name_;
        std::mutex mutex_;

        size_t exec_thread_num_;
        size_t idle_thread_num_;

        pid_t root_threadID_;

        std::vector<std::jthread> thread_list_;
        std::list<Task::ptr> task_list_;

        size_t thread_num_;
        Coroutine::ptr root_routine_;

        bool running_;

        Epoll epoller_;

        static thread_local Coroutine *current_root_routine_;
        static thread_local Scheduler *current_schedule_;
    };

} // namespace xb

#endif
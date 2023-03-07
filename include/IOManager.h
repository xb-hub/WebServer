//
// Created by 许斌 on 2022/11/5.
//
#ifndef _IOMANAGER_H_
#define _IOMANAGER_H_
#include <vector>
#include <sys/epoll.h>
#include "Scheduler.h"
#include "Timer.h"
#include "Log.h"

namespace xb
{

    enum FdEventType
    {
        NONE = 0x0,
        READ = 0x1, // EPOLLIN
        WRITE = 0x4 // EPOLLOUT
    };

    struct FdContext
    {
        // using ptr = std::shared_ptr<FdContext>;
        struct EventHandler // 事件处理
        {
            Scheduler *scheduler = nullptr; // 协程调度器
            std::function<void()> func;     // 事件回调函数
            Coroutine::ptr routine;         // 事件处理协程
        };

        FdContext(int fd);

        MutexLock mutex_;                      // 互斥锁
        int fd_;                               // 事件文件描述符
        FdEventType type_ = FdEventType::NONE; // 事件类型
        EventHandler read_handler_;
        EventHandler write_handler_;

        EventHandler &getHandler(FdEventType type);
        void resetHandler(EventHandler &handler);
        void triggerEvent(FdEventType type);
    };

    class IOManager : public Scheduler, public TimerManager
    {

    public:
        using ptr = std::shared_ptr<IOManager>;
        IOManager(size_t thread_num = 1, bool use_call = true, const std::string &name = "");
        ~IOManager();

        int addEvent(int fd, FdEventType event, std::function<void()> callback = nullptr);
        bool removeEvent(int fd, FdEventType event_type);
        bool cancelEvent(int fd, FdEventType event_type);
        bool cancelAll(int fd);

        void onIdel() override;
        bool CanStop() override;
        void tickle() override;
        void onTimerInsertedAtFront() override;
        bool stopping(uint64_t &timeout);

        static IOManager *GetThis();

    protected:
        void resizeList(size_t size);
        // bool stopping(uint64_t& timeout);
        void dealconnection(int fd);

    private:
        int epollfd_;
        int pipefd_[2];

        MutexLock mutex_;

        const int MAX_EVENTS_NUM_;
        std::vector<FdContext *> fd_context_list_;
        std::atomic_uint32_t evnet_num_;
    };

} // namespace xb

#endif //_IOMANAGER_H_

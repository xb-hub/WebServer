//
// Created by 许斌 on 2022/11/5.
//
#ifndef _IOMANAGER_H_
#define _IOMANAGER_H_
#include <vector>
#include <sys/epoll.h>
#include "Coroutine/Scheduler.h"

namespace xb
{

enum FdEventType
{
    NONE = 0x0,
    READ = 0x1,
    WRITE = 0x4
};

struct FdContext
{
    using ptr = std::shared_ptr<FdContext>;
    struct EventHandler     // 事件处理
    {
        Scheduler* schedule;            // 协程调度器
        std::function<void()> func;     // 事件回调函数
        Coroutine::ptr routine;         // 事件处理协程
    };

    FdContext(int fd);
    
    MutexLock mutex_;                    // 互斥锁
    int fd_;                             // 事件文件描述符
    FdEventType type_ = FdEventType::NONE;            // 事件类型
    EventHandler read_handler_;
    EventHandler write_handler_;
    FdEventType event_type_;

    EventHandler getHandler(FdEventType type);
    void resetHandler(EventHandler& handler);
    void triggerEvent(FdEventType type);
};


class IOManager : public Scheduler
{

public:
    IOManager(size_t thread_num, bool use_call, const std::string& name);
    ~IOManager();

    void addEventListener(int fd);
    void removeEventListener();

    void onIdel() override;
    bool isStop() override;
    void tick() override;

private:
    void resizeList(size_t size);

private:
    int epollfd_;
    const int MAX_EVENTS_NUM_;
    std::vector<FdContext::ptr> fd_context_list_;
    std::atomic_uint32_t evnet_num_;
};


} // namespace xb


#endif //_IOMANAGER_H_

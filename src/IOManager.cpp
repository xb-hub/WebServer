//
// Created by 许斌 on 2022/11/5.
//

#include <fcntl.h>
#include "IOManager.h"

namespace xb
{

FdContext::FdContext(int fd) :
    fd_(fd)
{}

FdContext::EventHandler& FdContext::getHandler(FdEventType type)
{
    switch (type)
    {
    case FdEventType::READ:
        return read_handler_;
        break;
    case FdEventType::WRITE:
        return write_handler_;
        break;
    default:
        break;
    }
}

void FdContext::resetHandler(FdContext::EventHandler& handler)
{
    handler.scheduler = nullptr;
    handler.func = nullptr;
    handler.routine.reset();
}

void FdContext::triggerEvent(FdEventType type)
{
    assert(type & type_);
    type_ = static_cast<FdEventType>(type_ & ~type);
    EventHandler& handler = getHandler(type);
    if(handler.func)
    {
        handler.scheduler->addTask(handler.func);
    }
    if(handler.routine)
    {
        handler.scheduler->addTask(handler.routine);
    }
    handler.scheduler = nullptr;
    return;
}

IOManager::IOManager(size_t thread_num, bool use_call, const std::string& name) :
    evnet_num_(0),
    MAX_EVENTS_NUM_(256),
    Scheduler(thread_num, use_call, name)
{
    epollfd_ = epoll_create(5000);
    int rt = pipe(pipefd_);
    assert(!rt);

    epoll_event event{0};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = pipefd_[0];

    rt = fcntl(pipefd_[0], F_SETFL, O_NONBLOCK);
    assert(!rt);

    rt = epoll_ctl(epollfd_, EPOLL_CTL_ADD, pipefd_[0], &event);
    assert(!rt);
    resizeList(64);
    start();
}

IOManager::~IOManager()
{
    stop();
    close(epollfd_);
    close(pipefd_[0]);
    close(pipefd_[1]);

    for(size_t i = 0; i < fd_context_list_.size(); i++)
    {
        if(fd_context_list_[i]) delete fd_context_list_[i];
    }
}

IOManager* IOManager::GetThis()
{
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

int IOManager::addEvent(int fd, FdEventType event, std::function<void()> callback)
{
    LOG_DEBUG(stdout_logger, "调用 IOManager::addEvent()");
    FdContext* fd_ctx = nullptr;
    {
        MutexLockGuard lock(mutex_);
        if(fd >= fd_context_list_.size())
        {
            resizeList(fd_context_list_.size() * 2);
        }
        fd_ctx = fd_context_list_[fd];
    }
    
    MutexLockGuard lock(fd_ctx->mutex_);
    int op = fd_ctx->type_ ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    // 创建事件
    epoll_event ev{0};
    ev.events = EPOLLET | fd_ctx->type_ | event;
    ev.data.ptr = fd_ctx;
    // 给 fd 注册事件监听
    int rt = epoll_ctl(epollfd_, op, fd, &ev);
    if(rt)
    {
        LOG_DEBUG(stdout_logger, "事件注册失败");
        return -1;
    }
    LOG_FMT_DEBUG(stdout_logger, "事件注册 sock=%d", fd);
    ++evnet_num_;
    
    fd_ctx->type_ = static_cast<FdEventType>(fd_ctx->type_ | event);
    FdContext::EventHandler& event_handler = fd_ctx->getHandler(event);
    
    // 确保没有给这个 fd 没有重复添加事件监听
    assert(!event_handler.scheduler && !event_handler.func && !event_handler.routine);
    event_handler.scheduler = Scheduler::GetThis();
    if (callback)
    {
        event_handler.func.swap(callback);
    }
    else
    {
        event_handler.routine = Coroutine::GetThis();
    }
    return 0;
}

bool IOManager::removeEvent(int fd, FdEventType type)
{
    FdContext* fd_ctx = nullptr;
    {
        MutexLockGuard lock(mutex_);
        if(fd_context_list_.size() <= fd) {
            return false;
        }
        fd_ctx = fd_context_list_[fd];
    }

    MutexLockGuard lock(fd_ctx->mutex_);
    if(!(fd_ctx->type_ & type)) {
        return false;
    }

    FdEventType new_events = static_cast<FdEventType>(fd_ctx->type_ & ~type);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event ev{0};
    ev.events = EPOLLET | new_events;
    ev.data.ptr = fd_ctx;

    int rt = epoll_ctl(epollfd_, op, fd, &ev);
    if(rt) {
        return false;
    }

    --evnet_num_;
    fd_ctx->type_ = new_events;
    FdContext::EventHandler& event_ctx = fd_ctx->getHandler(type);
    fd_ctx->resetHandler(event_ctx);
    return true;
}

bool IOManager::cancelEvent(int fd, FdEventType type)
{
    FdContext* fd_ctx = nullptr;
    {
        MutexLockGuard lock(mutex_);
        if(fd_context_list_.size() <= fd) {
            return false;
        }
        fd_ctx = fd_context_list_[fd];
    }

    MutexLockGuard lock(fd_ctx->mutex_);
    if(!(fd_ctx->type_ & type)) {
        return false;
    }

    FdEventType new_events = static_cast<FdEventType>(fd_ctx->type_ & ~type);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event ev{0};
    ev.events = EPOLLET | new_events;
    ev.data.ptr = fd_ctx;

    int rt = epoll_ctl(epollfd_, op, fd, &ev);
    if(rt) {
        return false;
    }
    fd_ctx->triggerEvent(type);
    --evnet_num_;
    return true;
}
bool IOManager::cancelAll(int fd)
{
    FdContext* fd_ctx = nullptr;
    {
        MutexLockGuard lock(mutex_);
        if(fd_context_list_.size() <= fd) {
            return false;
        }
        fd_ctx = fd_context_list_[fd];
    }

    MutexLockGuard lock(fd_ctx->mutex_);
    if(!fd_ctx->type_) {
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event ev{0};
    ev.events = 0;
    ev.data.ptr = fd_ctx;

    int rt = epoll_ctl(epollfd_, op, fd, &ev);
    if(rt) {
        return false;
    }

    if(fd_ctx->type_ & READ) {
        fd_ctx->triggerEvent(READ);
        --evnet_num_;
    }
    if(fd_ctx->type_ & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --evnet_num_;
    }

    assert(fd_ctx->type_ == 0);
    return true;
}

void IOManager::resizeList(size_t size)
{
    size_t start = fd_context_list_.size();
    fd_context_list_.resize(size);
    for(size_t i = start; i < size; i++)
    {
        if(!fd_context_list_[i])
        {
            fd_context_list_[i] = new FdContext(i);
        }
    }
}

void IOManager::tickle()
{
    if(!hasIdleThread()) return;
    int ret = write(pipefd_[1], "T", 1);
    assert(ret == 1);
}

bool IOManager::CanStop()
{
    uint64_t timeout = 0;
    return stopping(timeout);
}

bool IOManager::stopping(uint64_t& timeout)
{
    timeout = getNextTimer();
    return timeout == ~0ull && (evnet_num_ == 0) && Scheduler::CanStop();
}

void IOManager::onIdel()
{
    LOG_DEBUG(stdout_logger, "调用 IOManager::onIdle()");
    epoll_event events[MAX_EVENTS_NUM_];
    while(true)
    {
        uint64_t next_timeout = 0;
        if(stopping(next_timeout)) {
            LOG_INFO(stdout_logger, " idle stopping exit");
            break;
        }
        int rt = 0;
        while(true)
        {
            static const int MAX_TIMEOUT = 3000;
            if(next_timeout != ~0ull) {
                next_timeout = (int)next_timeout > MAX_TIMEOUT
                                ? MAX_TIMEOUT : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            // LOG_INFO(stdout_logger, " epoll wait");
            rt = epoll_wait(epollfd_, events, MAX_EVENTS_NUM_, (int)next_timeout);
            if(rt == -1 && errno == EINTR)
            {
                // 处理异常
            }
            else    break;
        }

        std::vector<std::function<void()> > timers;
        listExpiredCb(timers); // 获取超时任务
        if (!timers.empty()) {
            addTask(timers.begin(), timers.end());   // 函数添加到调度器中
            timers.clear();
        }

        for(size_t i = 0; i < rt; i++)
        {
            if(events[i].data.fd == pipefd_[0]) {
                uint8_t dummy[256];
                while(read(pipefd_[0], dummy, sizeof(dummy)) > 0);
                continue;
            }
            FdContext* fd_ctx = static_cast<FdContext*>(events[i].data.ptr);
            MutexLockGuard lock(fd_ctx->mutex_);
            if(events[i].events & (EPOLLERR | EPOLLHUP))
            {
                events[i].events |= (EPOLLIN | EPOLLOUT) & fd_ctx->type_;
            }
            u_int32_t real_event = FdEventType::NONE;
            if(events[i].events & EPOLLIN)
            {
                real_event |= FdEventType::READ;
            }
            if(events[i].events & EPOLLOUT)
            {
                real_event |= FdEventType::WRITE;
            }
            if(!(fd_ctx->type_ & real_event))
            {
                LOG_DEBUG(stdout_logger, "continue");
                continue;
            }
            u_int32_t type = (fd_ctx->type_ & ~real_event);
            int op = type ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            events[i].events = EPOLLET | type;
            rt = epoll_ctl(epollfd_, op, fd_ctx->fd_, &events[i]);
            // LOG_DEBUG(stdout_logger, "epoll MOD");
            if(rt)
            {
                LOG_DEBUG(stdout_logger, "modify failure");
                continue;
            }

            if(real_event & FdEventType::READ)
            {
                // LOG_DEBUG(stdout_logger, "epoll read");
                fd_ctx->triggerEvent(FdEventType::READ);
                --evnet_num_;
            }
            if(real_event & FdEventType::WRITE)
            {
                // LOG_DEBUG(stdout_logger, "epoll write");
                fd_ctx->triggerEvent(FdEventType::WRITE);
                --evnet_num_;
            }
        }
        Coroutine::ptr cur = Coroutine::GetThis();
        Coroutine* ptr = cur.get();
        cur.reset();
        ptr->swapOut();
    }
}

void IOManager::onTimerInsertedAtFront()
{
    tickle();
}

} // namespace xb

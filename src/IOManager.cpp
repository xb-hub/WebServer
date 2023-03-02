//
// Created by 许斌 on 2022/11/5.
//

#include "IOManager.h"

namespace xb
{

FdContext::FdContext(int fd) :
    fd_(fd)
{}

FdContext::EventHandler FdContext::getHandler(FdEventType type)
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
    handler.schedule = nullptr;
    handler.func = nullptr;
    handler.routine.reset();
}

void FdContext::triggerEvent(FdEventType type)
{
    assert(type && event_type_);
    event_type_ = static_cast<FdEventType>(event_type_ & ~type);
    EventHandler handler = getHandler(type);
    if(handler.func)
    {
        handler.schedule->addTask(handler.func);
    }
    if(handler.routine)
    {
        handler.schedule->addTask(handler.routine);
    }
}


IOManager::IOManager(size_t thread_num, bool use_call, const std::string& name) :
    evnet_num_(0),
    MAX_EVENTS_NUM_(256),
    Scheduler(thread_num, use_call, name)
{
    epollfd_ = epoll_create(0xffff);

    resizeList(64);
    start();
}

void IOManager::addEventListener(int fd)
{
    if(fd > fd_context_list_.size())
    {
        resizeList(fd_context_list_.size() * 2);
    }
    
}

void IOManager::removeEventListener()
{
    
}

void IOManager::resizeList(size_t size)
{
    size_t start = fd_context_list_.size();
    fd_context_list_.resize(size);
    for(size_t i = start; i < size; i++)
    {
        if(!fd_context_list_[i])
        {
            fd_context_list_[i] = std::make_shared<FdContext>(i);
        }
    }
}

void IOManager::onIdel()
{
    std::cout << "调用 IOManager::onIdle()" << std::endl;
    epoll_event events[MAX_EVENTS_NUM_];
    while(true)
    {
        int ret = 0;
        while(true)
        {
            // 计算定时器

            ret = epoll_wait(epollfd_, events, MAX_EVENTS_NUM_, 5000);
            if(ret < 0)
            {
                // 处理异常
            }
            else    break;
        }

        for(size_t i = 0; i < ret; i++)
        {
            if(events[i].data.fd == 1)
            {
                
            }
            auto fd_ctx = static_cast<FdContext*>(events[i].data.ptr);
            MutexLockGuard lock(fd_ctx->mutex_);
            if(events[i].events & (EPOLLERR | EPOLLHUP))
            {
                events[i].events |= EPOLLIN | EPOLLOUT;
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
            if(!(fd_ctx->event_type_ & real_event))
            {
                continue;
            }
            u_int32_t type = (fd_ctx->event_type_ & ~real_event);
            int op = type ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            events[i].events = EPOLLET | type;
            int rt = epoll_ctl(epollfd_, op, fd_ctx->fd_, &events[i]);
            if(rt == -1)
            {
                std::cout << "modify failure" << std::endl;
            }
            // if(events[i].events == FdEventType::NONE)
            // {
            //     int rt = epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd_ctx->fd, &events[i]);
            //     if(rt == -1)
            //     {
            //         std::cout << "modify failure" << std::endl;
            //     }
            // }
            if(events[i].events & FdEventType::READ)
            {
                fd_ctx->triggerEvent(FdEventType::READ);
            }
            if(events[i].events & FdEventType::WRITE)
            {
                fd_ctx->triggerEvent(FdEventType::WRITE);
            }
        }
        Coroutine::ptr cur = Coroutine::GetThis();
        Coroutine* current_fiber = cur.get();
        cur.reset();
        current_fiber->swapOut();
    }
}

} // namespace xb

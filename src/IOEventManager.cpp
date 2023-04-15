//
// Created by 许斌 on 2022/11/5.
//

#include <fcntl.h>
#include "IOEventManager.h"

namespace xb
{
    IOEventManager::IOEventManager()
        : epoll_(2048),
        timermanager_(std::make_shared<TimerManager>())
        //   Scheduler(thread_num, use_call, name)
    {
    }

    IOEventManager::~IOEventManager()
    {
        
    }

    void IOEventManager::addEvent(IOEvent::ptr event)
    {
        // LOG_DEBUG(stdout_logger, "调用 IOEventManager::addEvent()");
        int fd = event->getEventFd();
        fd_event_list_.insert({fd, event});
        epoll_.CtlEvent(event.get(), EPOLL_CTL_ADD);
        event_num_++;
    }

    void IOEventManager::removeEvent(IOEvent::ptr event)
    {
        if(!event)   return;
        if(fd_event_list_.find(event->getEventFd()) != fd_event_list_.end())
        {
            fd_event_list_.erase(event->getEventFd());
            epoll_.CtlEvent(event.get(), EPOLL_CTL_DEL);
            event_num_--;
        }
    }

    // void IOEventManager::removeEvent(int fd)
    // {
    //     if(fd_event_list_.find(fd) != fd_event_list_.end())
    //     {
    //         fd_event_list_.erase(fd);
    //         epoll_.CtlEvent(event.get(), EPOLL_CTL_DEL);
    //         event_num_--;
    //     }
    // }

    void IOEventManager::modifyEvent(IOEvent::ptr event)
    {
        int fd = event->getEventFd();
        auto iter = fd_event_list_.find(fd);
        if(iter != fd_event_list_.end())
        {
            iter->second = event;
            epoll_.CtlEvent(event.get(), EPOLL_CTL_MOD);
        }
        else
        {
            addEvent(event);
        }
    }

    void IOEventManager::addEvent(int fd, std::function<void()> func, uint32_t event_type)
    {
    }


    std::shared_ptr<IOEvent> IOEventManager::getFdEvent(int fd)
    {
        auto iter = fd_event_list_.find(fd);
        if(iter == fd_event_list_.end())
            fd_event_list_[fd] = std::make_shared<IOEvent>(fd);
        return fd_event_list_[fd];
    }

    void IOEventManager::waitEventAndHandler()
    {
        uint64_t timeout = timermanager_->getNextTimer();
        int rt = epoll_.Wait(timeout);
        timermanager_->listExpiredCb(timers); // 获取超时任务
        for(auto &it : timers)
        {
            // scheduler_->addTask(it);
            it();
        }
        timers.clear();
        if(rt > 0)
        {
            for(size_t i = 0; i < rt; i++)
            {
                int fd = epoll_.GetEventFd(i);
                std::shared_ptr<IOEvent> ioEvent = fd_event_list_[fd];
                if(ioEvent)
                {
                    // LOG_FMT_INFO(GET_ROOT_LOGGER(), "epoll deal [%d]: [%d]", epoll_.getEpollFd(), fd);
                    ioEvent->triggerEvent(ioEvent->getEventType());
                }
                else
                {
                    removeEvent(ioEvent);
                }
            }
        }
    }

    // void IOEventManager::onIdel()
    // {
    //     LOG_DEBUG(stdout_logger, "调用 IOEventManager::onIdle()");
    //     epoll_event events[MAX_EVENTS_NUM_];
    //     while (true)
    //     {
    //         uint64_t next_timeout = 0;
    //         if (stopping(next_timeout))
    //         {
    //             LOG_INFO(stdout_logger, " idle stopping exit");
    //             break;
    //         }
    //         int rt = 0;
    //         while (true)
    //         {
    //             static const int MAX_TIMEOUT = 3000;
    //             if (next_timeout != ~0ull)
    //             {
    //                 next_timeout = (int)next_timeout > MAX_TIMEOUT
    //                                    ? MAX_TIMEOUT
    //                                    : next_timeout;
    //             }
    //             else
    //             {
    //                 next_timeout = MAX_TIMEOUT;
    //             }
    //             // LOG_INFO(stdout_logger, " epoll wait");
    //             rt = epoll_wait(epollfd_, events, MAX_EVENTS_NUM_, (int)next_timeout);
    //             if (rt == -1 && errno == EINTR)
    //             {
    //                 // 处理异常
    //             }
    //             else
    //                 break;
    //         }

    //         std::vector<std::function<void()>> timers;
    //         listExpiredCb(timers); // 获取超时任务
    //         if (!timers.empty())
    //         {
    //             addTask(timers.begin(), timers.end()); // 函数添加到调度器中
    //             timers.clear();
    //         }

    //         for (size_t i = 0; i < rt; i++)
    //         {
    //             if (events[i].data.fd == pipefd_[0])
    //             {
    //                 uint8_t dummy[256];
    //                 while (read(pipefd_[0], dummy, sizeof(dummy)) > 0) {}
    //                 continue;
    //             }
    //             FdContext *fd_ctx = static_cast<FdContext *>(events[i].data.ptr);
    //             MutexLockGuard lock(fd_ctx->mutex_);
    //             if (events[i].events & (EPOLLERR | EPOLLHUP))
    //             {
    //                 events[i].events |= (EPOLLIN | EPOLLOUT) & fd_ctx->type_;
    //             }
    //             u_int32_t real_event = FdEventType::NONE;
    //             if (events[i].events & EPOLLIN)
    //             {
    //                 real_event |= FdEventType::READ;
    //             }
    //             if (events[i].events & EPOLLOUT)
    //             {
    //                 real_event |= FdEventType::WRITE;
    //             }
    //             if (!(fd_ctx->type_ & real_event))
    //             {
    //                 LOG_DEBUG(stdout_logger, "continue");
    //                 continue;
    //             }
    //             u_int32_t type = (fd_ctx->type_ & ~real_event);
    //             int op = type ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    //             events[i].events = EPOLLET | type;
    //             rt = epoll_ctl(epollfd_, op, fd_ctx->fd_, &events[i]);
    //             LOG_DEBUG(stdout_logger, "epoll MOD");
    //             if (rt)
    //             {
    //                 LOG_DEBUG(stdout_logger, "modify failure");
    //                 continue;
    //             }

    //             if (real_event & FdEventType::READ)
    //             {
    //                 // LOG_DEBUG(stdout_logger, "epoll read");
    //                 fd_ctx->triggerEvent(FdEventType::READ);
    //                 --evnet_num_;
    //             }
    //             if (real_event & FdEventType::WRITE)
    //             {
    //                 // LOG_DEBUG(stdout_logger, "epoll write");
    //                 fd_ctx->triggerEvent(FdEventType::WRITE);
    //                 --evnet_num_;
    //             }
    //         }
    //         Coroutine::ptr cur = Coroutine::GetThis();
    //         Coroutine *ptr = cur.get();
    //         cur.reset();
    //         ptr->swapOut();
    //     }
    // }

} // namespace xb

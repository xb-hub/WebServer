#include "Epoll.h"
#include "Log.h"

namespace xb
{
    Epoll::Epoll(int maxEvent)
        : epollFd_(epoll_create(512)),
          events_list_(maxEvent)
    {
        LOG_FMT_DEBUG(GET_ROOT_LOGGER(), "CREATE EPOLL [%d]", epollFd_);
        assert(epollFd_ >= 0 && events_list_.size() > 0);
    }

    Epoll::~Epoll()
    {
        close(epollFd_);
    }

    int Epoll::CtlEvent(IOEvent* event, int op)
    {
        int fd = event->getEventFd();
        return CtlFd(fd, op, event->getEventType());
    }

    int Epoll::CtlFd(int fd, int op, uint32_t events)
    {
        struct epoll_event event{0};
        event.data.fd = fd;
        event.events = events | EPOLLET;
        int rt = epoll_ctl(epollFd_, op, fd, &event);
        return rt;
    }

    bool Epoll::AddFd(int fd, uint32_t events)
    {
        if (fd < 0)
            return false;
        if(CtlFd(fd, EPOLL_CTL_ADD, events) < 0)
        {
            LOG_DEBUG(GET_ROOT_LOGGER(), "Epoll Add fd failure!");
            return false;
        }
        return true;
    }

    bool Epoll::ModFd(int fd, uint32_t events)
    {
        if (fd < 0)
            return false;
        if(CtlFd(fd, EPOLL_CTL_MOD, events) < 0)
        {
            LOG_DEBUG(GET_ROOT_LOGGER(), "Epoll Add fd failure!");
            return false;
        }
        return true;
    }

    bool Epoll::DelFd(int fd)
    {
        if (fd < 0)
            return false;
        if(CtlFd(fd, EPOLL_CTL_DEL, 0) < 0)
        {
            LOG_DEBUG(GET_ROOT_LOGGER(), "Epoll Add fd failure!");
            return false;
        }
        return true;
    }

    int Epoll::Wait(int timeoutMs)
    {
        int rt = epoll_wait(epollFd_, &events_list_[0], static_cast<int>(events_list_.size()), timeoutMs);
        return rt;
    }

    int Epoll::GetEventFd(size_t i) const
    {
        assert(i < events_list_.size() && i >= 0);
        return events_list_[i].data.fd;
    }

    uint32_t Epoll::GetEvents(size_t i) const
    {
        assert(i < events_list_.size() && i >= 0);
        return events_list_[i].events;
    }
} // namespace xb

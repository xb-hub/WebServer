/*
 * @Author       : mark
 * @Date         : 2020-06-15
 * @copyleft Apache 2.0
 */
#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <errno.h>

#include "IOEvent.h"

namespace xb
{
    class Epoll
    {
    public:
        explicit Epoll(int maxEvent = 1024);

        ~Epoll();

        int getEpollFd() const { return epollFd_; }


        int CtlEvent(IOEvent* event, int op);
        int CtlFd(int fd, int op, uint32_t events);

        bool AddFd(int fd, uint32_t events);

        bool ModFd(int fd, uint32_t events);

        bool DelFd(int fd);

        bool AddEvent(IOEvent* evnet);

        bool ModEvent(IOEvent* event);

        int Wait(int timeoutMs = -1);

        int GetEventFd(size_t i) const;

        uint32_t GetEvents(size_t i) const;

    private:
        int epollFd_;

        std::vector<struct epoll_event> events_list_;
    };
} // namespace xb

#endif // EPOLLER_H
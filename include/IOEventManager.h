//
// Created by 许斌 on 2022/11/5.
//
#ifndef _IOEVENTMANAGER_H_
#define _IOEVENTMANAGER_H_
#include <map>
#include <sys/epoll.h>
#include "Scheduler.h"
#include "Timer.h"
#include "Log.h"
#include "IOEvent.h"
#include "Epoll.h"
#include "Scheduler.h"

namespace xb
{

    class IOEventManager : public TimerManager
    {

    public:
        using ptr = std::shared_ptr<IOEventManager>;
        IOEventManager();
        ~IOEventManager();

        void addEvent(IOEvent::ptr event);
        void removeEvent(IOEvent::ptr event);
        void modifyEvent(IOEvent::ptr event);

        void addEvent(int fd, std::function<void()> func, uint32_t event_type);
        void waitEventAndHandler();

        std::shared_ptr<IOEvent> getFdEvent(int fd);

        int getEpollFd() const { return epoll_.getEpollFd(); }

    protected:

    private:
        Epoll epoll_;
        TimerManager::ptr timermanager_;
        std::vector<std::function<void()>> timers;
        Scheduler::ptr scheduler_;

        std::map<int, std::shared_ptr<IOEvent>> fd_event_list_;
        std::atomic_uint32_t event_num_{0};
    };

} // namespace xb

#endif //_IOEVENTMANAGER_H_

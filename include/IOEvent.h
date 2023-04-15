#ifndef _IOEVENT_H_
#define _IOEVENT_H_
#include <memory>
#include <functional>
#include <sys/epoll.h>

namespace xb
{
    class Epoll;

    class IOEvent
    {
    public:
        using ptr = std::shared_ptr<IOEvent>;
        using HandlerFunc = std::function<void()>;

        IOEvent(int fd);
        ~IOEvent();

        void setReadEventHandler(const HandlerFunc& func) { readEventHandler_ = func; }
        void setWriteEventHandler(const HandlerFunc& func) { writeEventHandler_ = func; }
        void setErrorEventHandler(const HandlerFunc& func) { errorEventHandler_ = func; }
        void setCloseEventHandler(const HandlerFunc& func) { closeEventHandler_ = func; }

        int getEventFd() const { return eventfd_; }
        uint32_t getEventType() const { return event_type_; }

        void enableReading() { event_type_ |= EPOLLIN; }
        void enableWriting() { event_type_ |= EPOLLOUT; }

        HandlerFunc getEventHandler(uint32_t event_type);
        void resetEventHandler();
        void triggerEvent(uint32_t event_type);
    
    private:
        int eventfd_;
        uint32_t event_type_;

        HandlerFunc readEventHandler_;
        HandlerFunc writeEventHandler_;
        HandlerFunc errorEventHandler_;
        HandlerFunc closeEventHandler_;
    };

    
} // namespace xb


#endif
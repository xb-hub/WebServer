#include "IOEvent.h"
#include "Log.h"

namespace xb
{
    IOEvent::IOEvent(int fd)
        : eventfd_(fd),
          event_type_(0x0)
    {

    }

    IOEvent::~IOEvent()
    {

    }

    IOEvent::HandlerFunc IOEvent::getEventHandler(uint32_t event_type)
    {
        HandlerFunc handler_func;
        if ((event_type & EPOLLHUP) && !(event_type & EPOLLIN))
        {
            //  LOG_DEBUG(GET_ROOT_LOGGER(), "EPOLLHUP");
            handler_func = closeEventHandler_;
        }
        if (event_type & EPOLLERR)
        {
            //  LOG_DEBUG(GET_ROOT_LOGGER(), "EPOLLERR");
            handler_func = errorEventHandler_;
        }
        if (event_type & (EPOLLIN))
        {
            //  LOG_DEBUG(GET_ROOT_LOGGER(), "EPOLLIN");
            handler_func = readEventHandler_;
        }
        if (event_type & EPOLLOUT)
        {
            //  LOG_DEBUG(GET_ROOT_LOGGER(), "EPOLLOUT");
            handler_func = writeEventHandler_;
        }
        return handler_func;
    }

    inline void IOEvent::resetEventHandler()
    {
        readEventHandler_ = nullptr;
        writeEventHandler_ = nullptr;
        errorEventHandler_ = nullptr;
        closeEventHandler_ = nullptr;
    }

    void IOEvent::triggerEvent(uint32_t event_type)
    {
        IOEvent::HandlerFunc func = getEventHandler(event_type);
        if(func)
        {
            func();
        }
        else
        {
            LOG_DEBUG(GET_ROOT_LOGGER(), "No CallBack!");
        }
    }

} // namespace xb

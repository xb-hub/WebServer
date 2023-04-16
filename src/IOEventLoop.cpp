#include "IOEventLoop.h"

namespace xb
{
    IOEventLoop::IOEventLoop()
        : iomanager_(std::make_shared<IOEventManager>())
    {}

    IOEventLoop::~IOEventLoop()
    {}

    void IOEventLoop::AddEvent(IOEvent::ptr event)
    {
        iomanager_->addEvent(event);
    }

    void IOEventLoop::ModEvent(IOEvent::ptr event)
    {
        iomanager_->modifyEvent(event);
    }

    void IOEventLoop::DelEvent(IOEvent::ptr event)
    {
        iomanager_->removeEvent(event);
    }

    void IOEventLoop::AddEvent(std::function<void()> func, uint32_t event_type)
    {
        // iomanager_->addEvent
    }

    void IOEventLoop::ModEvent(std::function<void()> func, uint32_t event_type)
    {

    }

    void IOEventLoop::DelEvent(uint32_t event_type)
    {

    }

    void IOEventLoop::run()
    {
        LOG_DEBUG(GET_ROOT_LOGGER(), "Thread Run");
        while(true)
        {
            iomanager_->waitEventAndHandler();
            runAllFunctionInLoop();
        }
    }

    void IOEventLoop::runAllFunctionInLoop()
    {
        for(auto &it : function_list_)
        {
            it();
        }
        function_list_.clear();
    }


} // namespace xb

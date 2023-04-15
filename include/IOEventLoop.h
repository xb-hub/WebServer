#ifndef _IOEVENTLOOP_H_
#define _IOEVENTLOOP_H_
#include "Timer.h"
#include "IOEventManager.h"

namespace xb
{

    class IOEventLoop
    {
    
    public:
        using ptr = std::shared_ptr<IOEventLoop>;
        using LoopFunc = std::function<void()>;
        IOEventLoop(/* args */);
        ~IOEventLoop();

        void AddEvent(IOEvent::ptr event);
        void ModEvent(IOEvent::ptr event);
        void DelEvent(IOEvent::ptr event);

        void AddEvent(std::function<void()> func, uint32_t event_type);
        void ModEvent(std::function<void()> func, uint32_t event_type);
        void DelEvent(uint32_t event_type);

        void run();

        void runAllFunctionInLoop();

        void addFunctionInLoop();

        std::shared_ptr<IOEvent> getFdEvent(int fd) { return iomanager_->getFdEvent(fd); }
        int getEpollFd() const { return iomanager_->getEpollFd(); }

    private:
        /* data */
        IOEventManager::ptr iomanager_;
        
        std::vector<LoopFunc> function_list_;

    };
    
} // namespace xb

#endif

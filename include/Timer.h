#ifndef _TIMER_H_
#define _TIMER_H_

#include <memory>
#include <vector>
#include <set>
#include "Log.h"

namespace xb
{
    class TimerManager;

    class Timer : public std::enable_shared_from_this<Timer>
    {
        friend class TimerManager;

    public:
        using ptr = std::shared_ptr<Timer>;
        using TimerFunc = std::function<void()>;
        Timer(uint64_t ms, TimerFunc func, bool recurring, TimerManager *manager);
        Timer(uint64_t next);
        ~Timer() = default;

        bool cancel();
        bool refresh();
        bool reset(uint64_t ms, bool now);

    private:
        uint64_t ms_;
        uint64_t next_;
        bool recurring_ = false;
        TimerFunc func_;

        TimerManager *manager_ = nullptr;

        struct Comparator
        {
            bool operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const;
        };
    };

    class TimerManager
    {
        friend class Timer;

    public:
        using ptr = std::shared_ptr<TimerManager>;
        using unptr = std::unique_ptr<TimerManager>;
        TimerManager(/* args */);
        virtual ~TimerManager();

        void addTimer(Timer::ptr time);

        virtual void onTimerInsertedAtFront();

        uint64_t getNextTimer();
        void listExpiredCb(std::vector<std::function<void()>> &func);
        bool detectClockRollover(uint64_t now_ms);
        bool hasTimer();

        Timer::ptr addTimer(uint64_t ms, std::function<void()> func, bool recurring = false);
        Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> func, std::weak_ptr<void> weak_cond, bool recurring = false);

    private:
        std::mutex mutex_;
        std::set<Timer::ptr, Timer::Comparator> timer_list_;
        // 是否触发onTimerInsertedAtFront
        bool tickle_ = false;

        uint64_t m_previouseTime = 0;
    };

} // namespace xb

#endif
#include "Timer.h"
#include "util.h"

namespace xb
{
    static xb::Logger::ptr stdout_logger = GET_ROOT_LOGGER();

    bool Timer::Comparator::operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const
    {
        if (!lhs && !rhs)
            return false;
        else if (!lhs)
            return true;
        else if (!rhs)
            return false;
        else if (lhs->next_ < rhs->next_)
            return true;
        else if (lhs->next_ > rhs->next_)
            return false;
        return lhs.get() < rhs.get();
    }

    Timer::Timer(uint64_t ms, TimerFunc func, bool recurring, TimerManager *manager)
        : ms_(ms),
          func_(func),
          recurring_(recurring),
          manager_(manager)
    {
        next_ = GetCurrentMS() + ms_;
    }

    Timer::Timer(uint64_t next) : next_(next)
    {
    }

    // Timer::~Timer()
    // {}

    bool Timer::cancel()
    {
        MutexLockGuard lock(manager_->mutex_);
        if (func_)
        {

            func_ = nullptr;
            auto it = manager_->timer_list_.find(shared_from_this());
            manager_->timer_list_.erase(it);
            return true;
        }
        return false;
    }

    bool Timer::refresh()
    {
        MutexLockGuard lock(manager_->mutex_);
        if (!func_)
        {
            return false;
        }
        auto it = manager_->timer_list_.find(shared_from_this());
        if (it == manager_->timer_list_.end())
        {
            return false;
        }
        manager_->timer_list_.erase(it);
        next_ = GetCurrentMS() + ms_;
        manager_->timer_list_.insert(shared_from_this());
        return true;
    }

    bool Timer::reset(uint64_t ms, bool now)
    {
        if (ms == ms_ && !now)
            return true;
        MutexLockGuard lock(manager_->mutex_);
        if (!func_)
            return false;
        auto it = manager_->timer_list_.find(shared_from_this());
        if (it == manager_->timer_list_.end())
        {
            return false;
        }
        manager_->timer_list_.erase(it);
        uint64_t start = 0;
        if (now)
        {
            start = GetCurrentMS();
        }
        else
        {
            start = next_ - ms_;
        }
        ms_ = ms;
        next_ = start + ms;
        manager_->addTimer(shared_from_this());
        return true;
    }

    TimerManager::TimerManager()
    {
        m_previouseTime = GetCurrentMS();
    }

    TimerManager::~TimerManager()
    {
    }

    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> func, bool recurring)
    {
        Timer::ptr timer(new Timer(ms, func, recurring, this));
        MutexLockGuard lock(mutex_);
        addTimer(timer);
        return timer;
    }

    static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb)
    {
        std::shared_ptr<void> tmp = weak_cond.lock();
        if (tmp)
        {
            cb();
        }
    }

    Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> func, std::weak_ptr<void> weak_cond, bool recurring)
    {
        return addTimer(ms, std::bind(&OnTimer, weak_cond, func), recurring);
    }

    void TimerManager::addTimer(Timer::ptr time)
    {
        auto it = timer_list_.insert(time).first;
        bool at_front = (it == timer_list_.begin()) && !tickle_;
        if (at_front)
        {
            tickle_ = true;
        }

        if (at_front)
        {
            onTimerInsertedAtFront();
        }
    }

    uint64_t TimerManager::getNextTimer()
    {
        MutexLockGuard lock(mutex_);
        if (timer_list_.empty())
            return ~0ull;
        Timer::ptr time = *timer_list_.begin();
        uint64_t now_ms = GetCurrentMS();
        if (time->next_ <= now_ms)
            return ~0ull;
        return time->next_ - now_ms;
    }

    void TimerManager::listExpiredCb(std::vector<std::function<void()>> &func)
    {
        uint64_t now_ms = GetCurrentMS();
        std::vector<Timer::ptr> expired;
        {
            MutexLockGuard lock(mutex_);
            if (timer_list_.empty())
                return;
            bool rollover = detectClockRollover(now_ms);
            if (!rollover && ((*timer_list_.begin())->next_ > now_ms))
            {
                return;
            }

            Timer::ptr now_timer(new Timer(now_ms));
            auto it = rollover ? timer_list_.end() : timer_list_.lower_bound(now_timer);
            while (it != timer_list_.end() && (*it)->next_ >= now_ms)
            {
                ++it;
            }
            expired.insert(expired.begin(), timer_list_.begin(), it);
            timer_list_.erase(timer_list_.begin(), it);
        }
        func.reserve(expired.size());

        for (auto &timer : expired)
        {
            func.push_back(timer->func_);
            if (timer->recurring_)
            {
                timer->next_ = now_ms + timer->ms_;
                timer_list_.insert(timer);
            }
            else
            {
                timer->func_ = nullptr;
            }
        }
    }

    bool TimerManager::detectClockRollover(uint64_t now_ms)
    {
        bool rollover = false;
        if (now_ms < m_previouseTime &&
            now_ms < (m_previouseTime - 60 * 60 * 1000))
        {
            rollover = true;
        }
        m_previouseTime = now_ms;
        return rollover;
    }

    bool TimerManager::hasTimer()
    {
        MutexLockGuard lock(mutex_);
        return !timer_list_.empty();
    }

    void TimerManager::onTimerInsertedAtFront()
    {

    }

} // namespace xb

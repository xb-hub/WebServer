//
// Created by 许斌 on 2022/9/5.
//

#ifndef _THREAD_H_
#define _THREAD_H_

#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include <functional>
#include <memory>
#include <boost/noncopyable.hpp>
#include <atomic>

namespace xb
{

    class MutexLock : public boost::noncopyable
    {
    public:
        MutexLock()
        {
            pthread_mutex_init(&mutex_, nullptr);
        }

        ~MutexLock()
        {
            pthread_mutex_destroy(&mutex_);
        }

        void lock()
        {
            pthread_mutex_lock(&mutex_);
        }

        void unlock()
        {
            pthread_mutex_unlock(&mutex_);
        }

        pthread_mutex_t *getMutex()
        {
            return &mutex_;
        }

    private:
        pthread_mutex_t mutex_;
    };

    class MutexLockGuard : public boost::noncopyable
    {
    public:
        explicit MutexLockGuard(MutexLock &mutex) : mutex_(mutex)
        {
            mutex_.lock();
        }

        ~MutexLockGuard()
        {
            mutex_.unlock();
        }

    private:
        MutexLock &mutex_;
    };

    class Condition : public boost::noncopyable
    {
    public:
        explicit Condition(MutexLock &mutex) : mutex_(mutex)
        {
            pthread_cond_init(&cond_, nullptr);
        }

        ~Condition()
        {
            pthread_cond_destroy(&cond_);
        }

        void wait()
        {
            //    std::cout << "wait" << std::endl;
            pthread_cond_wait(&cond_, mutex_.getMutex());
        }

        void notify()
        {
            pthread_cond_signal(&cond_);
        }

        void notifyAll()
        {
            //    std::cout << "broadcast" << std::endl;
            pthread_cond_broadcast(&cond_);
        }

    private:
        pthread_cond_t cond_;
        MutexLock &mutex_;
    };

    class RWLock : public boost::noncopyable
    {
    public:
        RWLock()
        {
            pthread_rwlock_init(&rw_lock_, nullptr);
        }

        ~RWLock()
        {
            pthread_rwlock_destroy(&rw_lock_);
        }

        void rdLock()
        {
            pthread_rwlock_rdlock(&rw_lock_);
        }

        void wrLock()
        {
            pthread_rwlock_wrlock(&rw_lock_);
        }

        void unlock()
        {
            pthread_rwlock_unlock(&rw_lock_);
        }

    private:
        pthread_rwlock_t rw_lock_;
    };

    class ReadLockGraud : public boost::noncopyable
    {
    public:
        ReadLockGraud(RWLock &r_lock) : r_lock_(r_lock)
        {
            r_lock_.rdLock();
        }

        ~ReadLockGraud()
        {
            r_lock_.unlock();
        }

    private:
        RWLock &r_lock_;
    };

    class WriteLockGraud : public boost::noncopyable
    {
    public:
        WriteLockGraud(RWLock &w_lock) : w_lock_(w_lock)
        {
            w_lock_.wrLock();
        }

        ~WriteLockGraud()
        {
            w_lock_.unlock();
        }

    private:
        RWLock &w_lock_;
    };

    class CountDownLatch : public boost::noncopyable
    {
    public:
        explicit CountDownLatch(int count) : mutex_(),
                                             cond_(mutex_),
                                             count_(count)
        {
        }

        void wait()
        {
            MutexLockGuard lock(mutex_);
            while (count_ > 0)
                cond_.wait();
        }

        void countDown()
        {
            MutexLockGuard lock(mutex_);
            --count_;
            if (count_ == 0)
            {
                cond_.notifyAll();
            }
        }

        int getCount() const
        {
            MutexLockGuard lock(mutex_);
            return count_;
        }

    private:
        mutable MutexLock mutex_;
        Condition cond_;
        int count_;
    };

    class Thread : boost::noncopyable
    {
    public:
        using ptr = std::shared_ptr<Thread>;
        using ThreadFunc = std::function<void()>;
        Thread(const std::string &name, ThreadFunc func);
        ~Thread();

        pid_t getId() const { return tid_; }
        void setName(const std::string &name) { name_ = name; }
        std::string getName() const { return name_; }
        void join();

        void start();

    private:
        static void *run(void *arg);

    private:
        pid_t tid_ = -1;
        pthread_t threadId_ = 0;
        std::string name_;
        ThreadFunc func_;

        bool started_;
        bool join_;
        CountDownLatch latch_;
    };

    class ThreadData : boost::noncopyable
    {
    public:
        using ptr = std::shared_ptr<ThreadData>;
        using ThreadFunc = Thread::ThreadFunc;

        ThreadData(const std::string &name, ThreadFunc func, pid_t *tid, CountDownLatch *latch);
        ~ThreadData();

        void runInThread();

    private:
        std::string name_;
        ThreadFunc func_;
        pid_t *tid_;
        CountDownLatch *latch_;

        // static std::atomic_uint32_t threadNum_;
    };

} // namespace xb

#endif // _THREAD_H_

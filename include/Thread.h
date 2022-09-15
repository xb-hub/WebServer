//
// Created by 许斌 on 2022/9/5.
//

#ifndef EPOLL_H_THREAD_H
#define EPOLL_H_THREAD_H

#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include <boost/noncopyable.hpp>

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

    pthread_mutex_t* getMutex()
    {
        return &mutex_;
    }

private:
    pthread_mutex_t mutex_;
};

class MutexLockGuard : public boost::noncopyable
{
public:
    explicit MutexLockGuard(MutexLock& mutex) :
        mutex_(mutex)
    {
        mutex_.lock();
    }

    ~MutexLockGuard()
    {
        mutex_.unlock();
    }

private:
    MutexLock& mutex_;
};

class Condition : public boost::noncopyable
{
public:
    explicit Condition(MutexLock& mutex):
        mutex_(mutex)
    {
        pthread_cond_init(&cond_, nullptr);
    }

    ~Condition()
    {
        pthread_cond_destroy(&cond_);
    }

    void wait()
    {
//        std::cout << "wait" << std::endl;
        pthread_cond_wait(&cond_, mutex_.getMutex());
    }

    void notify()
    {
        pthread_cond_signal(&cond_);
    }

    void notifyAll()
    {
//        std::cout << "broadcast" << std::endl;
        pthread_cond_broadcast(&cond_);
    }

private:
    pthread_cond_t cond_;
    MutexLock& mutex_;
};

class RWLock : public boost::noncopyable
{
public:
    RWLock()
    {
        pthread_rwlock_init(&m_rw_lock, nullptr);
    }

    ~RWLock()
    {
        pthread_rwlock_destroy(&m_rw_lock);
    }

    void rdLock()
    {
        pthread_rwlock_rdlock(&m_rw_lock);
    }

    void wrLock()
    {
        pthread_rwlock_wrlock(&m_rw_lock);
    }

    void unlock()
    {
        pthread_rwlock_unlock(&m_rw_lock);
    }

private:
    pthread_rwlock_t m_rw_lock;
};

class ReadLockGraud : public boost::noncopyable
{
public:
    ReadLockGraud(RWLock& r_lock) :
            m_r_lock(r_lock)
    {
        m_r_lock.rdLock();
    }

    ~ReadLockGraud()
    {
        m_r_lock.unlock();
    }

private:
    RWLock& m_r_lock;
};

class WriteLockGraud : public boost::noncopyable
{
public:
    WriteLockGraud(RWLock& w_lock) :
            m_w_lock(w_lock)
    {
        w_lock.wrLock();
    }

    ~WriteLockGraud()
    {
        m_w_lock.unlock();
    }

private:
    RWLock& m_w_lock;
};

class CountDownLatch : public boost::noncopyable
{
public:

    explicit CountDownLatch(int count) :
        mutex_(),
        cond_(mutex_),
        count_(count)
    {
    }

    void wait()
    {
        MutexLockGuard lock(mutex_);
        while(count_ > 0)   cond_.wait();
    }

    void countDown()
    {
        MutexLockGuard lock(mutex_);
        --count_;
        if(count_ == 0)
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

class Thread
{
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(const std::string name, std::function<void()> cb);
    ~Thread();

    const std::string& getName() const  { return m_name; }
    void join();

private:
    static void* run(void* arg);

private:
    pthread_t m_thread = 0;
    std::string m_name;
    std::function<void()> m_cb;
};

#endif //EPOLL_H_THREAD_H

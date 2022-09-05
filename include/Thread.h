//
// Created by 许斌 on 2022/9/5.
//

#ifndef EPOLL_H_THREAD_H
#define EPOLL_H_THREAD_H

#include <pthread.h>
#include <iostream>

class MutexLock
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

class MutexLockGuard
{
public:
    explicit MutexLockGuard(MutexLock& mutex) :
        mutex_(mutex)
    {
//        std::cout << "lock" << std::endl;
        mutex_.lock();
    }

    ~MutexLockGuard()
    {
//        std::cout << "unlock" << std::endl;
        mutex_.unlock();
    }

private:
    MutexLock& mutex_;
};

class Condition
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

class CountDownLatch
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

#endif //EPOLL_H_THREAD_H

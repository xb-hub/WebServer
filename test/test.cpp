//
// Created by 许斌 on 2022/9/5.
//

#include "Thread.h"
#include <iostream>

MutexLock mutex_;
Condition cond_(mutex_);
//pthread_mutex_t mutex_;
//pthread_cond_t cond_;
int n = 1, count = 0;

void* consumer(void*)
{
    while (1)
    {
        MutexLockGuard lock(mutex_);
//        pthread_mutex_lock(&mutex_);
        while(count <= 0)
        {
            cond_.wait();
//            pthread_cond_wait(&cond_, &mutex_);
        }
        std::cout << ")";
        count--;
        cond_.notifyAll();
//        pthread_cond_broadcast(&cond_);
//        pthread_mutex_unlock(&mutex_);
    }
}

void* producer(void*)
{
    while (1)
    {
        MutexLockGuard lock(mutex_);
//        pthread_mutex_lock(&mutex_);
        while(count >= n)
        {
            cond_.wait();
//            pthread_cond_wait(&cond_, &mutex_);
        }
        std::cout << "(";
        count++;
//        pthread_cond_broadcast(&cond_);
//        pthread_mutex_unlock(&mutex_);
        cond_.notifyAll();
    }
}

int main()
{
    std::cout << &mutex_ << std::endl;
//    pthread_mutex_init(&mutex_, nullptr);
//    pthread_cond_init(&cond_, nullptr);
    pthread_t tid;
    for(int i = 0; i < 2; i++)
    {
        pthread_create(&tid, nullptr, producer, nullptr);
        pthread_create(&tid, nullptr, consumer, nullptr);
    }
    pthread_join(tid, nullptr);
}
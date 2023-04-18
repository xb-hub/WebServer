//
// Created by 许斌 on 2022/9/5.
//

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

std::mutex mutex_;
std::condition_variable cond_;
// pthread_mutex_t mutex_;
// pthread_cond_t cond_;
int n = 1, count = 0;

void *consumer(void *)
{
    while (1)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        //        pthread_mutex_lock(&mutex_);
        while (count <= 0)
        {
            cond_.wait(lock);
            //            pthread_cond_wait(&cond_, &mutex_);
        }
        std::cout << ")";
        count--;
        cond_.notify_all();
        //        pthread_cond_broadcast(&cond_);
        //        pthread_mutex_unlock(&mutex_);
    }
}

void *producer(void *)
{
    while (1)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        //        pthread_mutex_lock(&mutex_);
        while (count >= n)
        {
            cond_.wait(lock);
            //            pthread_cond_wait(&cond_, &mutex_);
        }
        std::cout << "(";
        count++;
        //        pthread_cond_broadcast(&cond_);
        //        pthread_mutex_unlock(&mutex_);
        cond_.notify_all();
    }
}

int main()
{
    std::cout << &mutex_ << std::endl;
    //    pthread_mutex_init(&mutex_, nullptr);
    //    pthread_cond_init(&cond_, nullptr);
    pthread_t tid;
    for (int i = 0; i < 2; i++)
    {
        pthread_create(&tid, nullptr, producer, nullptr);
        pthread_create(&tid, nullptr, consumer, nullptr);
    }
    pthread_join(tid, nullptr);
}
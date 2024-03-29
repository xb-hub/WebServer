#include <iostream>
#include <unistd.h>
#include "ThreadPool.h"
using namespace xb;

class MyTask
{
public:
    MyTask() {}

    int run(int i, const char *p)
    {
        printf("thread[%lu] : (%d, %s)\n", pthread_self(), i, (char *)p);
        sleep(1);
        return 0;
    }
};

int main()
{
    ThreadPool::ptr pool = std::make_shared<ThreadPool>("WebSever");
    MyTask taskObj[20];
    for (int i = 0; i < 20; i++)
    {
        pool->AddTask(std::bind(&MyTask::run, &taskObj[i], i, "helloworld"));
    }

    while (1)
    {
        printf("there are still %d tasks need to process\n", pool->getSize());
        if (pool->getSize() == 0)
        {
            printf("Now I will exit from main\n");
            exit(0);
        }
        sleep(2);
    }

    return 0;
}
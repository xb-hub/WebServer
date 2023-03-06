#include "Log.h"
#include "Scheduler.h"
#include <iostream>

void fn()
{
    for (int i = 0; i < 3; i++)
    {
        std::cout << "啊啊啊啊啊啊" << std::endl;
        xb::Coroutine::Yield2Hold();
    }
}

void fn2()
{
    for (int i = 0; i < 3; i++)
    {
        std::cout << "哦哦哦哦哦哦" << std::endl;
        xb::Coroutine::Yield2Hold();
    }
}

int main(int, char**)
{
    xb::Scheduler sc(4, true);
    sc.start();

    int i = 0;
    for (i = 0; i < 3; i++)
    {
        sc.addTask([i]() {
            std::cout << ">>>>>> " << i << std::endl;
        });
        // sc.addTask(fn);
    }
    sc.stop();
    // sleep(5);
    return 0;
}

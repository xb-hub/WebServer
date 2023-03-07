//
// Created by 许斌 on 2022/9/9.
//

#include <iostream>
#include <sys/time.h>
#include "util.h"

namespace xb
{

    pid_t GetThreadId()
    {
        return static_cast<pid_t>(::syscall(SYS_gettid));
    }

    uint64_t GetCurrentMS()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000ul + tv.tv_usec / 1000ul;
    }

    uint64_t GetCurrentUS()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
    }

}

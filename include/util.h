//
// Created by 许斌 on 2022/9/9.
//

#ifndef _UTIL_H_
#define _UTIL_H_

#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <stdint.h>

namespace xb
{

    pid_t GetThreadId();

    uint64_t GetCurrentMS();

    uint64_t GetCurrentUS();

}

#endif //_UTIL_H_

//
// Created by 许斌 on 2022/9/9.
//

#ifndef _UTIL_H_
#define _UTIL_H_

#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>

namespace xb
{

pid_t GetThreadId();

}

#endif //_UTIL_H_

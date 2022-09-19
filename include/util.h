//
// Created by 许斌 on 2022/9/9.
//

#ifndef EPOLL_H_UTIL_H
#define EPOLL_H_UTIL_H

#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>

namespace xb
{

pid_t GetThreadId();

}

#endif //EPOLL_H_UTIL_H

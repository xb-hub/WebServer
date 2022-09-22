//
// Created by 许斌 on 2022/9/9.
//

#include <iostream>
#include "util.h"

namespace xb
{

pid_t GetThreadId()
{
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

}

//
// Created by 许斌 on 2022/9/9.
//

#include "util.h"

namespace xb
{

uint64_t GetThreadId()
{
    uint64_t id;
    pthread_threadid_np(0, &id);
    return id;
}

}

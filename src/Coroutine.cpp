#include "Coroutine.h"
#include <opencv2/opencv.hpp>

namespace xb
{
Schedule::Schedule()
{
}

Coroutine::Coroutine(CoroutineFunc func, uint64_t stack_size) :
        func_(std::move(func)),
        stack_size_(stack_size)
{}


} // namespace xb

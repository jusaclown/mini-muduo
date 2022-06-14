#pragma once

#include "src/Timer.h"

#include <memory.h>

class TimerId
{
    friend class TimerQueue;
public:
    TimerId()
        : timer_(nullptr)
        , sequence_(0)
    {}

    TimerId(std::shared_ptr<Timer> timer, int64_t seq)
        : timer_(std::move(timer))
        , sequence_(seq)
    {}
    
private:
    std::shared_ptr<Timer> timer_;
    int64_t sequence_;
};
#include "src/Timer.h"

std::atomic<int64_t> Timer::num_created_ = 0;

Timer::Timer(timer_callback cb, timer_clock::time_point when, timer_clock::duration interval)
    : callback_(std::move(cb))
    , expiration_(when)
    , interval_(interval)
    , repeat_(interval > timer_clock::duration::zero())
    , sequence_(++num_created_)
{}

void Timer::run() const
{
    callback_();
}

void Timer::restart(timer_clock::time_point now)
{
    if (repeat_)
    {
        expiration_ = now + interval_;
    }
    else
    {
        expiration_ = timer_clock::time_point::min();
    }
}
#include "src/Timer.h"

Timer::Timer(timer_callback cb, timer_clock::time_point when, timer_clock::duration interval)
    : callback_(std::move(cb))
    , expiration_(when)
    , interval_(interval)
    , repeat_(interval > timer_clock::duration::zero())
    , cancelled_(false)
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
        expiration_ = timer_clock::time_point();
    }
}
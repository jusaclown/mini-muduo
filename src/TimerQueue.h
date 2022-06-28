#pragma once

#include "src/common.h"
#include "src/Channel.h"
#include "src/Timer.h"

#include <map>
#include <vector>
#include <queue>

class EventLoop;
class Timer;

class TimerQueue
{
public:    
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    TimerQueue(const TimerQueue&) = delete;
    TimerQueue& operator=(const TimerQueue&) = delete;

    timer_ptr add_timer(timer_callback cb, timer_clock::time_point when, timer_clock::duration interval);
    void cancel(timer_ptr timer);

private:
    void handle_read_();
    std::vector<timer_ptr> get_expired_(timer_clock::time_point now);
    void reset_(const std::vector<timer_ptr>& expired, timer_clock::time_point now);
    bool insert_(timer_ptr timer);

    void add_timer_in_loop_(timer_ptr timer);
    void cancel_in_loop_(timer_ptr timer);

private:
    struct timer_cmp
    {
        bool operator()(const timer_ptr& p1, const timer_ptr p2)
        {
            return p1->expiration() > p2->expiration();
        }
    };

    EventLoop* loop_;
    const int timerfd_;                 /* timerfd_create 创建的定时器fd */
    Channel timerfd_channel_;           /* 定时器fd对应的Channel */
    std::priority_queue<timer_ptr, std::vector<timer_ptr>, timer_cmp> timers_;
};
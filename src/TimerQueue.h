#pragma once

#include "src/common.h"
#include "src/Channel.h"

#include <set>
#include <vector>

class EventLoop;
class Timer;
class TimerId;

class TimerQueue
{
public:
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    TimerId add_timer(timer_callback cb, timer_clock::time_point when, timer_clock::duration interval);
    void cancel(TimerId timerid);

private:
    using entry = std::pair<timer_clock::time_point, std::shared_ptr<Timer>>;
    using timer_set = std::set<entry>;
    using active_timer = std::pair<std::shared_ptr<Timer>, int64_t>;
    using active_timer_set = std::set<active_timer>;

    void handle_read_();
    std::vector<entry> get_expired_(timer_clock::time_point now);
    void reset_(const std::vector<entry>& expired, timer_clock::time_point now);
    bool insert_(std::shared_ptr<Timer> timer);

private:
    EventLoop* loop_;
    const int timerfd_;                 /* timerfd_create 创建的定时器fd */
    Channel timerfd_channel_;           /* 定时器fd对应的Channel */
    timer_set timers_;                  /* 按到期时间排序 */
    /* for cancel() */
    bool calling_expired_timers_;       /* 标识是否正在处理超时定时器回调函数*/
    active_timer_set active_timers_;    /* 按计时器地址排序，和timers_保存的其实是一个东西 */
    active_timer_set canceling_timers_;
};
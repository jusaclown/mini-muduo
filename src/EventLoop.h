#pragma once

#include "src/common.h"
#include "src/TimerId.h"

class EpollPoller;
class TimerQueue;

class EventLoop
{
public:
    EventLoop();
    ~EventLoop();

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    void loop();
    void update_channel(Channel* Channel);
    void quit();

    /* 定时器相关 */
    TimerId run_at(timer_clock::time_point time, timer_callback cb);
    TimerId run_after(timer_clock::duration delay, timer_callback cb);
    TimerId run_every(timer_clock::duration interval, timer_callback cb);
    void cancel(TimerId timerid);

private:
    bool quit_;
    std::unique_ptr<EpollPoller> poller_;
    channel_list active_channels_;              /* 由poller返回的活动Channel */
    std::unique_ptr<TimerQueue> timer_queue_;   /* 定时器队列 */
};
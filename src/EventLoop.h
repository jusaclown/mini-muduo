#pragma once

#include "src/common.h"

#include <atomic>
#include <mutex>

class EpollPoller;
class TimerQueue;
class Channel;

class EventLoop
{
public:
    using functor = std::function<void()>;
    
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

    void run_in_loop(functor cb);
    void queue_in_loop(functor cb);
    void wakeup();

    void assert_in_loop_thread()
    {
        if (!is_in_loop_thread())
        {
            abort_not_in_loop_thread_();
        }
    }
    bool is_in_loop_thread() const { return thread_id_ == thread_id(); }

private:
    bool abort_not_in_loop_thread_();
    void handle_read_();
    void do_pending_functors_();

private:
    std::atomic<bool> quit_;
    const size_t thread_id_;                    /* 线程id */
    std::unique_ptr<EpollPoller> poller_;
    channel_list active_channels_;              /* 由poller返回的活动Channel */
    std::unique_ptr<TimerQueue> timer_queue_;   /* 定时器队列 */

    int wakeupfd_;                              /* 用于唤醒线程的fd */
    std::unique_ptr<Channel> wakeup_channel_;   /* wakeupfd对应的Channel */
    
    mutable std::mutex mutex_;
    bool calling_pending_functors_;             /* 用于标识是否正在执行do_pending_functors_()函数 */
    std::vector<functor> pending_functors_;
};
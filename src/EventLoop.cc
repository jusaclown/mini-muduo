#include "src/EventLoop.h"
#include "src/EpollPoller.h"
#include "src/TimerQueue.h"

#include <signal.h>

#pragma GCC diagnostic ignored "-Wold-style-cast"
class IgnoreSigPipe
{
public:
    IgnoreSigPipe()
    {
        ::signal(SIGPIPE, SIG_IGN);
    }
};
#pragma GCC diagnostic error "-Wold-style-cast"
IgnoreSigPipe init_obj;

EventLoop::EventLoop()
    : quit_(false)
    , poller_(std::make_unique<EpollPoller>(this))
    , timer_queue_(std::make_unique<TimerQueue>(this))
{
    
}

EventLoop::~EventLoop()
{

}

void EventLoop::loop()
{
    // int count = 0;
    while (!quit_)
    {
        // ++count;
        active_channels_.clear();
        poller_->poller(active_channels_);

        /* 对所有活动Channel调用处理函数 */
        for (Channel* channel : active_channels_)
        {
            channel->handle_event();
        }

        // if (count >= 15)
        //     break;
    }
}

void EventLoop::update_channel(Channel* channel)
{
    poller_->update_channel(channel);
}

void EventLoop::quit()
{
    quit_ = true;
}

TimerId EventLoop::run_at(timer_clock::time_point time, timer_callback cb)
{
    return timer_queue_->add_timer(std::move(cb), time, timer_clock::duration::zero());
}

TimerId EventLoop::run_after(timer_clock::duration delay, timer_callback cb)
{
    return run_at(timer_clock::now() + delay, std::move(cb));
}

TimerId EventLoop::run_every(timer_clock::duration interval, timer_callback cb)
{
    return timer_queue_->add_timer(std::move(cb), timer_clock::now() + interval, interval);
}

void EventLoop::cancel(TimerId timerid)
{
    timer_queue_->cancel(timerid);
}
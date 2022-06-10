#include "src/EventLoop.h"
#include "src/EpollPoller.h"

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
#include "src/Channel.h"
#include "src/EventLoop.h"

#include <sys/epoll.h>

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
{ }

Channel::~Channel() = default;

void Channel::handle_event()
{
    if (revents_ & EPOLLIN)
    {
        if (read_callback_)
            read_callback_();
    }
}

void Channel::enable_reading()
{
    events_ |= EPOLLIN;
    update();
}

void Channel::update()
{
    loop_->update_channel(this);
}

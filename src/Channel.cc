#include "src/Channel.h"
#include "src/EventLoop.h"

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1)
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
    events_ |= kReadEvent;
    update_();
}

void Channel::disable_reading()
{
    events_ &= ~kReadEvent;
    update_();
}

void Channel::enable_writing()
{
    events_ |= kWriteEvent;
    update_();
}
void Channel::disable_writing()
{
    events_ &= ~kWriteEvent;
    update_();
}

void Channel::disable_all()
{
    events_ = kNoneEvent;
    update_();
}

bool Channel::is_writing() const
{
    return events_ & kWriteEvent;
}

bool Channel::is_reading() const
{
    return events_ & kReadEvent;
}

void Channel::update_()
{
    loop_->update_channel(this);
}

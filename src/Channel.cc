#include "src/Channel.h"
#include "src/TcpServer.h"

#include <sys/epoll.h>

Channel::Channel(TcpServer* tcp_ptr, int fd)
    : tcp_ptr_(tcp_ptr)
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
    tcp_ptr_->update_channel(this);
}

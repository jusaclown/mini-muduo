#include "src/Channel.h"
#include "src/EventLoop.h"

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1)
    , tied_(false)
{ }

Channel::~Channel() = default;

void Channel::handle_event()
{
    std::shared_ptr<void> guard;
    if (tied_)
    {
        if (guard = tie_.lock())
        {
            handle_event_with_guard_();
        }
    }
    else
    {
        handle_event_with_guard_();
    }
}

/**
 *  避免TcpConnection在handle_read时销毁
 *  当连接到来，创建一个TcpConnection对象，立刻用shared_ptr来管理，引用计数为1
 *  在Channel中维护一个weak_ptr(tie_)，将这个shared_ptr对象赋值给tie_，引用计数仍为1
 *  当连接关闭，在handleEvent，将tie_提升，得到一个shared_ptr对象，引用计数就变成了2
 */
void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied_ = true;
}

void Channel::update_()
{
    loop_->update_channel(this);
}

void Channel::handle_event_with_guard_()
{
    if (revents_ & EPOLLERR)
    {
        if (error_callback_)
            error_callback_();
    }
    
    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (read_callback_)
            read_callback_();
    }

    if (revents_ & EPOLLOUT)
    {
        if (write_callback_)
            write_callback_();
    }
}
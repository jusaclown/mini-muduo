#include "src/EpollPoller.h"

#include <unistd.h>
#include <sys/epoll.h>
#include <string.h>

EpollPoller::EpollPoller(EventLoop* loop)
    : loop_(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize)
{
    if (epollfd_ < 0)
        handle_err("epoll_create1()");
}

EpollPoller::~EpollPoller()
{
    ::close(epollfd_);
}

void EpollPoller::poller(channel_list& active_channels)
{    
    int nums = epoll_wait(epollfd_, events_.data(), static_cast<int>(events_.size()), -1);
    if (nums < 0)
        handle_err("epoll_wait");

    /* 将所有活动fd转为对应的Channel */
    for (int i = 0; i < nums; ++i)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        active_channels.push_back(channel);
    }

    if (events_.size() == static_cast<size_t>(nums))
    {
        events_.resize(events_.size() * 2);
    }
}

void EpollPoller::update_channel(Channel* channel)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.ptr = channel;
    ev.events = channel->events();
    if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, channel->fd(), &ev) < 0)
        handle_err("epoll_ctl");    
}
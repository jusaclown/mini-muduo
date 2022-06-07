#pragma once

#include "src/Channel.h"
#include "src/common.h"
#include "src/EventLoop.h"

#include <vector>
#include <map>

class EpollPoller
{
public:
    EpollPoller(EventLoop* loop);
    ~EpollPoller();

    EpollPoller(const EpollPoller&);
    EpollPoller& operator=(const EpollPoller&);

    void poller(channel_list& active_channels);
    void update_channel(Channel* Channel);

    
private:
    static const int kInitEventListSize = 16;

    EventLoop* loop_;       /* 所属EventLoop */
    int epollfd_;           /* epoll_create的文件描述符 */
    event_list events_;     /* epoll_wait填充的epoll_event数组 */
};
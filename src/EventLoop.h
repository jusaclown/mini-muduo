#pragma once

#include "src/common.h"

class EpollPoller;

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
    
private:
    bool quit_;
    std::unique_ptr<EpollPoller> poller_;
    channel_list active_channels_;         /* 由poller返回的活动Channel */
};
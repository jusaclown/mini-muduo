#pragma once

#include "src/common.h"

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

class EventLoop;

class EventLoopThread
{
public:
    EventLoopThread(thread_init_callback cb = thread_init_callback());
    ~EventLoopThread();

    EventLoopThread(const EventLoopThread&) = delete;
    EventLoopThread& operator=(const EventLoopThread&) = delete;
    
    EventLoop* start_loop();            // 启动线程，启动之后，该线程成为IO线程

private:
    void thread_func();

private:
    EventLoop* loop_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    thread_init_callback callback_;     // 回调函数在EventLoop::loop事件循环之前被调用
};
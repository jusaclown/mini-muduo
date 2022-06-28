#pragma once

#include "src/common.h"

#include <vector>
#include <memory>
#include <functional>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool
{
public:
    EventLoopThreadPool(EventLoop* baseloop);
    ~EventLoopThreadPool();

    EventLoopThreadPool(const EventLoopThreadPool&) = delete;
    EventLoopThreadPool& operator=(const EventLoopThreadPool&) = delete;

    void set_thread_num(int num_threads) { num_threads_ = num_threads; }
    void start(const thread_init_callback& cb = thread_init_callback());

    EventLoop* get_next_loop();
    EventLoop* get_loop_for_hash(size_t hash_code);

    std::vector<EventLoop*> get_all_loops();

    bool started() const { return started_; }

private:
    EventLoop* baseloop_;
    bool started_;
    int num_threads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};
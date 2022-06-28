#include "src/EventLoopThreadPool.h"
#include "src/EventLoopThread.h"

#include <cassert>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseloop)
    : baseloop_(baseloop)
    , started_(false)
    , num_threads_(0)
    , next_(0)
{}

EventLoopThreadPool::~EventLoopThreadPool()
{}

void EventLoopThreadPool::start(const thread_init_callback& cb)
{
    started_ = true;

    for (int i = 0; i < num_threads_; ++i)
    {
        EventLoopThread* t = new EventLoopThread(cb);
        threads_.emplace_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->start_loop());
    }
    if (num_threads_ == 0 && cb)
    {
        cb(baseloop_);
    }
}

EventLoop* EventLoopThreadPool::get_next_loop()
{
    EventLoop* loop = baseloop_;

    if (!loops_.empty())
    {
        loop = loops_[next_];
        ++next_;
        if (next_ == static_cast<int>(loops_.size()))
        {
            next_ = 0;
        }
    }

    return loop;
}

EventLoop* EventLoopThreadPool::get_loop_for_hash(size_t hash_code)
{
    EventLoop* loop = baseloop_;

    if (!loops_.empty())
    {
        loop = loops_[hash_code % loops_.size()];
    }

    return loop;    
}

std::vector<EventLoop*> EventLoopThreadPool::get_all_loops()
{
    assert(started_);
    if (loops_.empty())
    {
        return std::vector<EventLoop*>(1, baseloop_);
    }
    else
    {
        return loops_;
    }
}
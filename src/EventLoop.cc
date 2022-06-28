#include "src/EventLoop.h"
#include "src/EpollPoller.h"
#include "src/TimerQueue.h"
#include "src/Channel.h"

#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>

#pragma GCC diagnostic ignored "-Wold-style-cast"
class IgnoreSigPipe
{
public:
    IgnoreSigPipe()
    {
        ::signal(SIGPIPE, SIG_IGN);
    }
};
#pragma GCC diagnostic error "-Wold-style-cast"
IgnoreSigPipe init_obj;

/* 用来创建wakeupfd的函数 */
int create_eventfd()
{
    int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (fd < 0)
    {
        handle_err("eventfd");
    }
    return fd;
}

thread_local EventLoop* t_loop_in_this_thread = nullptr;

EventLoop::EventLoop()
    : quit_(false)
    , thread_id_(thread_id())
    , poller_(std::make_unique<EpollPoller>(this))
    , timer_queue_(std::make_unique<TimerQueue>(this))
    , wakeupfd_(create_eventfd())
    , wakeup_channel_(std::make_unique<Channel>(this, wakeupfd_))
    , calling_pending_functors_(false)
{
    if (t_loop_in_this_thread)
    {
        // 如果当前线程已经有一个EventLoop了，就报错
        printf("Another EventLoop exists in this thread\n");
        abort();
    }
    else
    {
        t_loop_in_this_thread = this;
    }

    wakeup_channel_->set_read_callback(std::bind(&EventLoop::handle_read_, this));
    wakeup_channel_->enable_reading();
}

EventLoop::~EventLoop()
{
    wakeup_channel_->disable_all();
    ::close(wakeupfd_);
    t_loop_in_this_thread = nullptr;
}

void EventLoop::loop()
{
    assert_in_loop_thread();
    quit_.store(false, std::memory_order_relaxed);
    
    while (!quit_)
    {

        active_channels_.clear();
        poller_->poller(active_channels_);

        /* 对所有活动Channel调用处理函数 */
        for (Channel* channel : active_channels_)
        {
            channel->handle_event();
        }
        
        do_pending_functors_();
    }
}

void EventLoop::update_channel(Channel* channel)
{
    assert_in_loop_thread();
    poller_->update_channel(channel);
}

void EventLoop::quit()
{
    quit_.store(true, std::memory_order_relaxed);
    if (!is_in_loop_thread())
    {
        wakeup();
    }
}

TimerId EventLoop::run_at(timer_clock::time_point time, timer_callback cb)
{
    return timer_queue_->add_timer(std::move(cb), time, timer_clock::duration::zero());
}

TimerId EventLoop::run_after(timer_clock::duration delay, timer_callback cb)
{
    return run_at(timer_clock::now() + delay, std::move(cb));
}

TimerId EventLoop::run_every(timer_clock::duration interval, timer_callback cb)
{
    return timer_queue_->add_timer(std::move(cb), timer_clock::now() + interval, interval);
}

void EventLoop::cancel(TimerId timerid)
{
    timer_queue_->cancel(timerid);
}

void EventLoop::run_in_loop(functor cb)
{
    // 如果是当前线程调用这个函数，则同步执行
    if (is_in_loop_thread())
    {
        cb();
    }
    // 如果在其他线程中调用这个函数，则将cb加入队列
    else
    {
        queue_in_loop(std::move(cb));
    }
}

void EventLoop::queue_in_loop(functor cb)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_functors_.push_back(std::move(cb));
    }

    /**
     * 调用queue_in_loop 的线程不是当前IO线程需要唤醒
     * 或者调用queue_in_loop的线程是当前IO线程，并且此时正在调用pending functor需要唤醒
     * 只有当前IO线程的事件回调中调用queue_in_loop才不需要唤醒
     */
    if (!is_in_loop_thread() || calling_pending_functors_)
    {
        wakeup();
    }
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeupfd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        printf("EventLoop::wakeup() writes %ld bytes instead of 8\n", n);
    }
}

bool EventLoop::abort_not_in_loop_thread_()
{
    printf("EventLoop::abortNotInLoopThread - EventLoop was created in thread_id_ = %ld, "
        "current thread_id = %ld\n", thread_id_, thread_id());
        abort();
}

void EventLoop::handle_read_()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupfd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        printf("EventLoop::handle_read_() writes %ld bytes instead of 8\n", n);
    }    
}

void EventLoop::do_pending_functors_()
{
    std::vector<functor> functors;
    calling_pending_functors_ = true;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pending_functors_);
    }

    for (const functor& f : functors)
    {
        f();
    }
    calling_pending_functors_ = false;
}
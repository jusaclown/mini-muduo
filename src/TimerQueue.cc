#include "src/TimerQueue.h"
#include "src/Timer.h"
#include "src/EventLoop.h"

#include <sys/timerfd.h>
#include <unistd.h>
#include <cassert>
#include <string.h>

int create_timerfd()
{
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    if (timerfd < 0)
        handle_err("timerfd_create()");

    return timerfd;
}

struct timespec time_point_to_timespec(timer_clock::time_point when)
{
    using namespace std::chrono;
    timer_clock::duration dura = when - timer_clock::now();
    auto secs = duration_cast<seconds>(dura);
    auto ns = duration_cast<nanoseconds>(dura) - duration_cast<nanoseconds>(secs);
    return timespec{secs.count(), ns.count()};
}

void reset_timerfd(int timerfd, timer_clock::time_point expiration)
{
    struct itimerspec new_value;
    struct itimerspec old_value;
    memset(&new_value, 0, sizeof(new_value));
    memset(&old_value, 0, sizeof(old_value));
    new_value.it_value = time_point_to_timespec(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &new_value, &old_value);
    if (ret)
    {
        printf("timerfd_settime(): %s\n", strerror(errno));
    }
}

void read_timerfd(int timerfd)
{
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof(howmany));
    if (n != sizeof(howmany))
    {
        printf("TimerQueue::handle_read_() reads %ld bytes instead of 8\n", n);
    }
}

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop)
    , timerfd_(create_timerfd())
    , timerfd_channel_(loop_, timerfd_)
{
    timerfd_channel_.set_read_callback(std::bind(&TimerQueue::handle_read_, this));
    timerfd_channel_.enable_reading();
}

TimerQueue::~TimerQueue()
{
    timerfd_channel_.disable_all();
    ::close(timerfd_);
}

timer_ptr TimerQueue::add_timer(timer_callback cb, timer_clock::time_point when, timer_clock::duration interval)
{
    auto timer = std::make_shared<Timer>(std::move(cb), when, interval);
    loop_->run_in_loop(std::bind(&TimerQueue::add_timer_in_loop_, this, timer));
    return timer;  
}

void TimerQueue::cancel(timer_ptr timer)
{
    loop_->run_in_loop(std::bind(&TimerQueue::cancel_in_loop_, this, std::move(timer)));
}

void TimerQueue::add_timer_in_loop_(timer_ptr timer)
{
    loop_->assert_in_loop_thread();
    bool earliest_changed = insert_(timer);
    if (earliest_changed)
    {
        /* ??????????????????????????????????????????????????????????????????????????? */
        reset_timerfd(timerfd_, timer->expiration());
    } 
}

void TimerQueue::cancel_in_loop_(timer_ptr timer)
{
    loop_->assert_in_loop_thread();
    // TODO: ???????????????????????????????????????????????????????????????????????????????????????????????????
    timer->set_cancelled(true);
}

void TimerQueue::handle_read_()
{
    /**
     * timerfd???????????????????????????????????????
     * 1. ????????????????????????????????????
     * 2. ?????????????????????????????????
     * 3. ?????????????????????????????????
     * 4. ????????????????????????
     */
    loop_->assert_in_loop_thread();
    timer_clock::time_point now = timer_clock::now();
    read_timerfd(timerfd_);
    std::vector<timer_ptr> expired = get_expired_(now);

    for (const timer_ptr& it : expired)
    {
        it->run();
    }
    reset_(expired, now);
}

std::vector<timer_ptr> TimerQueue::get_expired_(timer_clock::time_point now)
{
    std::vector<timer_ptr> expired;

    while (!timers_.empty())
    {
        auto&& timer = timers_.top();
        if (timer->cancelled())
        {
            timers_.pop();
            continue;
        }
        
        if (timer->expiration() > now)
        {
            break;
        }

        expired.push_back(timer);   /* how to use move? */
        timers_.pop();
    }

    return expired;
}

void TimerQueue::reset_(const std::vector<timer_ptr>& expired, timer_clock::time_point now)
{
    timer_clock::time_point next_expired;
    
    for (const timer_ptr& it : expired)
    {
        if (it->repeat())
        {
            it->restart(now);
            insert_(it);
        }
    }

    if (!timers_.empty())
    {
        next_expired = timers_.top()->expiration();
    }

    if (next_expired.time_since_epoch() > timer_clock::time_point::duration::zero())
    {
        reset_timerfd(timerfd_, next_expired);
    }
}

bool TimerQueue::insert_(timer_ptr timer)
{
    loop_->assert_in_loop_thread();
    bool earliest_changed = false;
    auto when = timer->expiration();
    
    /* ??????timers_????????????when??????timers_???????????????????????? */
    if (timers_.empty() || when < timers_.top()->expiration())
    {
        earliest_changed = true;
    }
    timers_.push(std::move(timer));

    return earliest_changed;
}

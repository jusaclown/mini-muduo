#include "src/TimerQueue.h"
#include "src/TimerId.h"
#include "src/Timer.h"

#include <sys/timerfd.h>
#include <unistd.h>
#include <cassert>

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
    , calling_expired_timers_(false)
{
    timerfd_channel_.set_read_callback(std::bind(&TimerQueue::handle_read_, this));
    timerfd_channel_.enable_reading();
}

TimerQueue::~TimerQueue()
{
    timerfd_channel_.disable_all();
    ::close(timerfd_);
}

TimerId TimerQueue::add_timer(timer_callback cb, timer_clock::time_point when, timer_clock::duration interval)
{
    auto timer = std::make_shared<Timer>(std::move(cb), when, interval);
    bool earliest_changed = insert_(timer);
    if (earliest_changed)
    {
        /* 如果插入的定时器最早到期，则需重置定时器的超时时刻 */
        reset_timerfd(timerfd_, timer->expiration());
    }
    return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerid)
{
    assert(timers_.size() == active_timers_.size());
    active_timer timer(timerid.timer_, timerid.sequence_);
    auto it = active_timers_.find(timer);
    if (it != active_timers_.end()) /* 找到了 */
    {
        size_t n = timers_.erase(entry(it->first->expiration(), it->first));
        assert(n == 1); (void)n;
        active_timers_.erase(it);
    }
    else if (calling_expired_timers_) /* 在定时器的回调函数中注销定时器 */
    {
        canceling_timers_.insert(timer);
    }
    assert(timers_.size() == active_timers_.size());
}

void TimerQueue::handle_read_()
{
    /**
     * timerfd可读，意味着有定时器到期了
     * 1. 读取该事件，避免一直触发
     * 2. 获取所有超时定时器列表
     * 3. 执行超时定时器回调函数
     * 4. 判断是否重复定时
     */
    timer_clock::time_point now = timer_clock::now();
    read_timerfd(timerfd_);
    std::vector<entry> expired = get_expired_(now);
    
    calling_expired_timers_ = true;
    canceling_timers_.clear();
    for (const entry& it : expired)
    {
        it.second->run();
    }
    calling_expired_timers_ = false;
    reset_(expired, now);
}

std::vector<TimerQueue::entry> TimerQueue::get_expired_(timer_clock::time_point now)
{
    std::vector<entry> expired;
    entry sentry(now + std::chrono::nanoseconds(1), nullptr);
    /**
     * 返回第一个未到期的Timer的迭代器
     * lower_bound的含义是返回第一个值 >= sentry的元素的iterator
     * 即 *end >= sentry, 从而 end->first > now;
     */
    timer_set::iterator end = timers_.lower_bound(sentry);
    assert(end == timers_.end() || now < end->first);
    /* 将到期的定时器插入到expired中 */
    std::copy(timers_.begin(), end, back_inserter(expired));
    /* 从timers_中移除到期的定时器 */
    timers_.erase(timers_.begin(), end);
    
    /* 从active_timers_中移除到期的定时器 */
    for (const entry& it : expired)
    {
        active_timer timer(it.second, it.second->sequence());
        active_timers_.erase(timer);
    }
    assert(timers_.size() == active_timers_.size());
    return expired;
}

void TimerQueue::reset_(const std::vector<entry>& expired, timer_clock::time_point now)
{
    timer_clock::time_point next_expired;
    
    for (const entry& it : expired)
    {
        active_timer timer(it.second, it.second->sequence());
        /* 如果是重复的定时器并且是未取消的定时器，则重启该定时器 */
        if (it.second->repeat()
            && canceling_timers_.find(timer) == canceling_timers_.end())
        {
            it.second->restart(now);
            insert_(it.second);
        }
    }

    if (!timers_.empty())
    {
        next_expired = timers_.begin()->second->expiration();
    }

    if (next_expired != timer_clock::time_point::min())
    {
        reset_timerfd(timerfd_, next_expired);
    }
}

bool TimerQueue::insert_(std::shared_ptr<Timer> timer)
{
    assert(timers_.size() == active_timers_.size());
    bool earliest_changed = false;
    auto when = timer->expiration();
    timer_set::iterator it = timers_.begin();
    /* 如果timers_为空或者when小于timers_中的最早到期时间 */
    if (it == timers_.end() || it->first > when)
    {
        earliest_changed = true;
    }
    {
        auto result = timers_.insert(entry(when, timer));
        assert(result.second); (void)result;
    }
    {
        auto result = active_timers_.insert(active_timer(timer, timer->sequence()));
        assert(result.second); (void)result;
    }

    assert(timers_.size() == active_timers_.size());

    return earliest_changed;
}
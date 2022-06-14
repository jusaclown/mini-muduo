#pragma once

#include "src/common.h"

#include <atomic>

class Timer
{
public:
    Timer(timer_callback cb, timer_clock::time_point when, timer_clock::duration interval);

    Timer(const Timer&) = delete;
    Timer operator=(const Timer&) = delete;

    void run() const;

    timer_clock::time_point expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    int64_t sequence() const { return sequence_; }
    static int64_t num_created() { return num_created_.load(); }
    
    void restart(timer_clock::time_point now);
    
private:
    const timer_callback callback_;             /* 定时器回调函数 */
    timer_clock::time_point expiration_;        /* 下一次的超时时刻 */
    const timer_clock::duration interval_;      /* 超时时间间隔，如果是一次性定时器，该值为0 */
    const bool repeat_;                         /* 是否重复，当interval_大于0时才为true */
    const int64_t sequence_;                    /* 定时器序号 */

    static std::atomic<int64_t> num_created_;   /* 定时器计数，当前已经创建的定时器数量 */
};
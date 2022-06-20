#pragma once

#include <functional>
#include <memory>
#include <sys/epoll.h>

class EventLoop;

class Channel
{
public:
    using event_callback = std::function<void()>;
    
    Channel(EventLoop* loop, int fd);
    ~Channel();

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    void handle_event();

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revents) { revents_ = revents; }
    bool is_none_event() const { return events_ == kNoneEvent; }

    void set_read_callback(event_callback cb)
    {
        read_callback_ = std::move(cb);
    }

    void set_write_callback(event_callback cb)
    {
        write_callback_ = std::move(cb);
    }
    
    void set_close_callback(event_callback cb)
    {
        close_callback_ = std::move(cb);
    }

    void set_error_callback(event_callback cb)
    {
        error_callback_ = std::move(cb);
    }

    void enable_reading()  { events_ |= kReadEvent; update_(); }
    void disable_reading() { events_ &= ~kReadEvent; update_(); }
    void enable_writing()  { events_ |= kWriteEvent; update_(); }
    void disable_writing() { events_ &= ~kWriteEvent; update_(); }
    void disable_all()     { events_ = kNoneEvent; update_(); }
    bool is_writing() const { return events_ & kWriteEvent; }
    bool is_reading() const { return events_ & kReadEvent; }

    int index() const { return index_; }
    void set_index(int idx) { index_ = idx; }

    /// Tie this channel to the owner object managed by shared_ptr,
    /// prevent the owner object being destroyed in handleEvent.
    void tie(const std::shared_ptr<void>&);

private:
    void update_();
    void handle_event_with_guard_();
    
private:
    static const int kNoneEvent = 0;
    static const int kReadEvent = EPOLLIN | EPOLLPRI;
    static const int kWriteEvent = EPOLLOUT;
    
    EventLoop* loop_;       /* 所属EventLoop */
    const int  fd_;         /* 文件描述符，但不负责关闭该文件描述符 */
    int        events_;     /* 用户关注的事件 */
    int        revents_;    /* epoll 返回的事件 */
    int        index_;      /* 表示在epoll的事件数组中的序号 */

    std::weak_ptr<void> tie_;
    bool tied_;

    event_callback read_callback_;
    event_callback write_callback_;
    event_callback close_callback_;
    event_callback error_callback_;
};
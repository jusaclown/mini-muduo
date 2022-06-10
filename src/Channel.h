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
        
    void set_read_callback(event_callback cb)
    {
        read_callback_ = std::move(cb);
    }

    void set_write_callback(event_callback cb)
    {
        write_callback_ = std::move(cb);
    }

    void enable_reading();
    void disable_reading();
    void enable_writing();
    void disable_writing();
    void disable_all();
    bool is_writing() const;
    bool is_reading() const;

    int index() const { return index_; }
    void set_index(int idx) { index_ = idx; }

private:
    void update_();

private:
    static const int kNoneEvent = 0;
    static const int kReadEvent = EPOLLIN;
    static const int kWriteEvent = EPOLLOUT;
    
    EventLoop* loop_;       /* 所属EventLoop */
    const int  fd_;         /* 文件描述符，但不负责关闭该文件描述符 */
    int        events_;     /* 用户关注的事件 */
    int        revents_;    /* epoll 返回的事件 */
    int        index_;      /* 表示在epoll的事件数组中的序号 */
    event_callback read_callback_;
    event_callback write_callback_;
};
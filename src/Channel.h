#pragma once

#include <functional>
#include <memory>

class TcpServer;

class Channel
{
public:
    using event_callback = std::function<void()>;
    
    Channel(TcpServer* tcp_ptr, int fd);
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
    
    void enable_reading();

private:
    void update();

private:
    TcpServer* tcp_ptr_;    /* 所属TcpServer */
    const int  fd_;         /* 文件描述符，但不负责关闭该文件描述符 */
    int        events_;     /* 用户关注的事件 */
    int        revents_;    /* epoll 返回的事件 */
    event_callback read_callback_;
};
#pragma once

#include "src/Channel.h"
#include "src/common.h"
#include "src/Buffer.h"

#include <netinet/in.h>
#include <memory>

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, int sockfd, const struct sockaddr_in& peer_addr);
    ~TcpConnection();

    TcpConnection(const TcpConnection&);
    TcpConnection& operator=(const TcpConnection&);

    void set_message_callback(message_callback cb)
    {
        message_callback_ = std::move(cb);
    }

    void set_close_callback(close_callback cb)
    {
        close_callback_ = std::move(cb);
    }

    void set_connection_callback(connection_callback cb)
    {
        connection_callback_ = std::move(cb);
    }
        
    int fd() const { return sockfd_; }
    struct sockaddr_in peer_addr() const { return peer_addr_; }

    void connect_established();

    void send(const std::string& message);

private:
    void handle_read_();    /* 可读事件的回调函数 */
    void handle_write_();   /* 可写事件的回调函数 */
    void handle_close_();

private:
    static const size_t kMaxBuffer = 1024;
    
    EventLoop* loop_;
    int sockfd_;
    std::unique_ptr<Channel> channel_;
    struct sockaddr_in peer_addr_;
    message_callback message_callback_;                 /* 消息到来 */
    close_callback close_callback_;                     /* 连接的断开 */
    connection_callback connection_callback_;           /* 连接的建立 */
    write_complete_callback write_complete_callback_;   /* 消息发送完毕 */
    Buffer input_buffer_;   /* 接收缓冲区 */
    Buffer output_buffer_;  /* 发送缓冲区 */
};
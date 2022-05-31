#pragma once

#include "src/Channel.h"
#include "src/common.h"

#include <netinet/in.h>
#include <memory>

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(TcpServer* tcp_ptr, int sockfd, const struct sockaddr_in& peer_addr);
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

    int fd() const { return sockfd_; }
    struct sockaddr_in peer_addr() const { return peer_addr_; }

private:
    void handle_read_();
    void handle_close_();

private:
    static const size_t kMaxBuffer = 1024;
    
    TcpServer* tcp_ptr_;
    int sockfd_;
    std::unique_ptr<Channel> channel_;
    struct sockaddr_in peer_addr_;
    message_callback message_callback_;
    close_callback close_callback_;
};
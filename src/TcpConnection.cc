#include "src/TcpConnection.h"

#include <unistd.h>
#include <string.h>

TcpConnection::TcpConnection(EventLoop* loop, int sockfd, const struct sockaddr_in& peer_addr)
    : loop_(loop)
    , sockfd_(sockfd)
    , channel_(std::make_unique<Channel>(loop_, sockfd_))
    , peer_addr_(peer_addr)
{
    channel_->set_read_callback(std::bind(&TcpConnection::handle_read_, this));
    channel_->enable_reading();
}

TcpConnection::~TcpConnection()
{
    if (sockfd_ != -1)
        ::close(sockfd_);
}

void TcpConnection::handle_read_()
{
    char buffer[kMaxBuffer];
    memset(buffer, 0, sizeof(buffer));
    ssize_t recv_nums = recv(sockfd_, buffer, kMaxBuffer - 1, 0);

    if (recv_nums <= 0)
    {
        handle_close_();
    }
    else
    {
        message_callback_(shared_from_this(), buffer, recv_nums);
    }
}

void TcpConnection::handle_close_()
{
    printf("close fd = %d\n", sockfd_);
    
    if (sockfd_ != -1)
        close(sockfd_);
    sockfd_ = -1;

    if (close_callback_)
        close_callback_(shared_from_this());
}
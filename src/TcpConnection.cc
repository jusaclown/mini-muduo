#include "src/TcpConnection.h"
#include "src/EventLoop.h"

#include <unistd.h>
#include <string.h>
#include <netinet/tcp.h>

TcpConnection::TcpConnection(EventLoop* loop, int sockfd, const struct sockaddr_in& peer_addr)
    : loop_(loop)
    , sockfd_(sockfd)
    , channel_(std::make_unique<Channel>(loop_, sockfd_))
    , peer_addr_(peer_addr)
    , state_(kConnecting)
{
    channel_->set_read_callback(std::bind(&TcpConnection::handle_read_, this));
    channel_->set_write_callback(std::bind(&TcpConnection::handle_write_, this));
    set_keep_alive(true);
}

TcpConnection::~TcpConnection()
{
    ::close(sockfd_);
}

void TcpConnection::connect_established()
{
    set_state(kConnected);
    channel_->enable_reading();
    connection_callback_(shared_from_this());
}

void TcpConnection::send(const std::string& message)
{
    ssize_t nwrote = 0;
    size_t remaining = message.size();
    /* 如果fd没有关注可写事件并且输出缓冲区无数据，则直接发送 
       ? !channel_->is_writing() 这里是针对什么情况，只用后面的判定不行吗？
     */
    if (!channel_->is_writing() && output_buffer_.readable_bytes() == 0)
    {
        nwrote = ::send(channel_->fd(), message.data(), message.size(), 0);
        if (nwrote >= 0)
        {
            remaining -= nwrote;
            if (remaining == 0 && write_complete_callback_)
            {
                write_complete_callback_(shared_from_this());
            }
        }
        else /* nwrote < 0 */
        {
            nwrote = 0;
            printf("There was a mistake(send_num != recv_nums), but we decided to continue\n");
        }
    }

    assert(nwrote >= 0);
    /* 如果没有发送完，则将剩余数据添加到输出缓冲区并关注可写事件 */
    if (remaining > 0)
    {
        output_buffer_.append(message.data() + nwrote, message.size() - nwrote);
        if (!channel_->is_writing())
            channel_->enable_writing();
    }
}

void TcpConnection::shutdown()
{
    if (connected())
    {
        set_state(kDisconnecting);
        if (!channel_->is_writing())
        {
            if (::shutdown(sockfd_, SHUT_WR) < 0)
            {
                // handle_err("shutdown()");
                printf("shutdown(): %s \n", strerror(errno));
            }
        }
    }
}

void TcpConnection::set_tcp_no_delay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof(optval)));
}

void TcpConnection::set_keep_alive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_,  SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof(optval)));
}

void TcpConnection::handle_read_()
{
    int saved_errno = 0;
    ssize_t recv_nums = input_buffer_.readfd(sockfd_, &saved_errno);

    if (recv_nums <= 0)
    {
        handle_close_();
    }
    else
    {
        message_callback_(shared_from_this(), input_buffer_);
    }
}

void TcpConnection::handle_write_()
{
    if (channel_->is_writing())
    {
        auto n = ::send(channel_->fd(), output_buffer_.peek(), output_buffer_.readable_bytes(), 0);
        if (n > 0)
        {
            output_buffer_.retrieve(n);
            /* 如果读完了，缓冲区没有数据了，取消关注可写事件 */
            if (output_buffer_.readable_bytes() == 0)
            {
                channel_->disable_writing();
                if (write_complete_callback_)
                {
                    write_complete_callback_(shared_from_this());
                }
            }
            else
            {
                printf("I am going to write more data\n");
            }
        }
        else
        {
            printf("send(): some error happened!\n");
        }
    }
}

void TcpConnection::handle_close_()
{
    set_state(kDisconnected);
    channel_->disable_all();

    auto ptr = shared_from_this();
    connection_callback_(ptr);
    close_callback_(ptr);
}
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
    
}

TcpConnection::~TcpConnection()
{
    if (sockfd_ != -1)
        ::close(sockfd_);
}

void TcpConnection::connect_established()
{
    channel_->enable_reading();
    connection_callback_(shared_from_this());
}

void TcpConnection::send(const std::string& message)
{
    ssize_t nwrote = 0;
    /* 如果fd没有关注可写事件并且输出缓冲区无数据，则直接发送 
       ? !channel_->is_writing() 这里是针对什么情况，只用后面的判定不行吗？
     */
    if (!channel_->is_writing() && output_buffer_.readable_bytes() == 0)
    {
        nwrote = ::send(channel_->fd(), message.data(), message.size(), 0);
        if (nwrote < 0)
        {
            nwrote = 0;
            printf("There was a mistake(send_num != recv_nums), but we decided to continue\n");
        }
    }

    assert(nwrote >= 0);
    /* 如果没有发送完，则将剩余数据添加到输出缓冲区并关注可写事件 */
    if (static_cast<size_t>(nwrote) < message.size())
    {
        output_buffer_.append(message.data() + nwrote, message.size() - nwrote);
        if (!channel_->is_writing())
            channel_->enable_writing();
    }
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
            }
            else
            {
                printf("I am going to write more data");
            }
        }
        else
        {
            printf("send(): some error happened!");
        }
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
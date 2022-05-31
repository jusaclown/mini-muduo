#include "src/TcpServer.h"
#include "src/common.h"
#include "src/Acceptor.h"
#include "TcpConnection.h"

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <stdlib.h>
#include <vector>

using namespace std::placeholders;

TcpServer::TcpServer()
    : TcpServer("0.0.0.0", 10086)
{}

TcpServer::TcpServer(std::string ip, uint16_t port)
    : acceptor_(std::make_unique<Acceptor>(this, std::move(ip), port))
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kMaxEvents)
{
    acceptor_->set_new_connection_callback(
        std::bind(&TcpServer::handle_listenfd_, this, _1, _2)
    );
}

TcpServer::~TcpServer()
{
    ::close(epollfd_);
}

void TcpServer::start()
{
    acceptor_->listen();

    // int count = 0;
    while (true)
    {
        // ++count;
        std::vector<Channel*> active_channels;
        int nums = epoll_wait(epollfd_, events_.data(), kMaxEvents, -1);

        if (nums < 0)
            handle_err("epoll_wait");

        /* 将所有活动fd转为对应的Channel */
        for (int i = 0; i < nums; ++i)
        {
            Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
            channel->set_revents(events_[i].events);
            active_channels.push_back(channel);
        }

        /* 对所有活动Channel调用处理函数 */
        for (Channel* channel : active_channels)
        {
            channel->handle_event();
        }

        // if (count >= 15)
        //     break;
    }
}

void TcpServer::update_channel(Channel* channel)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.ptr = channel;
    ev.events = channel->events();
    if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, channel->fd(), &ev) < 0)
        handle_err("epoll_ctl");    
}

void TcpServer::handle_listenfd_(int clientfd, struct sockaddr_in client_addr)
{
    auto conn = std::make_shared<TcpConnection>(this, clientfd, client_addr);
    conn->set_message_callback(std::bind(&TcpServer::handle_clientfd_, this, _1, _2, _3));
    conn->set_close_callback(std::bind(&TcpServer::remove_connection_, this, _1));
    connections_[clientfd] = conn;

    /* 打印新来的连接 */
    char buf[10];
    printf("new connection from [%s : %d], accept socket fd = %d\n",
        inet_ntop(AF_INET, &client_addr.sin_addr, buf, sizeof(buf)),
        ntohs(client_addr.sin_port),
        clientfd);
}

void TcpServer::handle_clientfd_(tcp_conn_ptr conn, char* buffer, ssize_t recv_nums)
{
    ssize_t send_num = send(conn->fd(), buffer, recv_nums, 0);
    if (send_num != recv_nums)
    {
        printf("There was a mistake(send_num != recv_nums), but we decided to continue\n");
    }
}

void TcpServer::remove_connection_(tcp_conn_ptr conn)
{
    connections_.erase(conn->fd());
}
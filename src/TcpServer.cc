#include "TcpServer.h"

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

#define handle_err(msg) do { perror(msg); exit(EXIT_FAILURE);} while(0);

TcpServer::TcpServer()
    : TcpServer("0.0.0.0", 8888)
{}

TcpServer::TcpServer(std::string ip, uint16_t port)
    : ip_(std::move(ip))
    , port_(port)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , listen_channel_(this, create_and_listen_())
    , events_(kMaxEvents)
{
    listen_channel_.set_read_callback(std::bind(&TcpServer::handle_listenfd_, this));
    listen_channel_.enable_reading();
}

TcpServer::~TcpServer()
{
    ::close(epollfd_);
    ::close(listen_channel_.fd());
}

void TcpServer::start()
{
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

int TcpServer::create_and_listen_()
{
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (sockfd < 0)
        handle_err("socket()");

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = static_cast<in_port_t>(htons(port_));
    inet_pton(AF_INET, ip_.c_str(), &server_address.sin_addr);
    // server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    int ret = bind(sockfd, reinterpret_cast<struct sockaddr*>(&server_address), sizeof(server_address));
    if (ret < 0)
        handle_err("bind()");
    
    ret = listen(sockfd, kMaxListen);
    if (ret < 0)
        handle_err("listen");
    
    return sockfd;
}

void TcpServer::handle_listenfd_()
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    int clientfd = accept4(listen_channel_.fd(),
        reinterpret_cast<struct sockaddr*>(&client_addr),
        &client_addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (clientfd < 0)
        handle_err("accept4");

    Channel* client_channel = new Channel(this, clientfd);
    client_channel->set_read_callback(std::bind(&TcpServer::handle_clientfd_, this, clientfd));
    client_channel->enable_reading();
    channels_[clientfd] = client_channel;

    /* 打印新来的连接 */
    char buf[10];
    printf("new connection from [%s : %d], accept socket fd = %d\n",
        inet_ntop(AF_INET, &client_addr.sin_addr, buf, sizeof(buf)),
        ntohs(client_addr.sin_port),
        clientfd);
}

void TcpServer::handle_clientfd_(int fd)
{
    char buffer[kMaxBuffer];
    memset(buffer, 0, sizeof(buffer));
    ssize_t recv_nums = recv(fd, buffer, kMaxBuffer - 1, 0);
    if (recv_nums <= 0)
    {
        close(fd);
        delete channels_[fd];
        channels_.erase(fd);
        printf("close fd = %d\n", fd);
    }
    else
    {
        ssize_t send_num = send(fd, buffer, recv_nums, 0);
        if (send_num != recv_nums)
        {
            printf("There was a mistake(send_num != recv_nums), but we decided to continue\n");
        }
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
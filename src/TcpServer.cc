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

#define handle_err(msg) do { perror(msg); exit(EXIT_FAILURE);} while(0);

TcpServer::TcpServer()
    : ip_("0.0.0.0")
    , port_(8888)
{}

TcpServer::TcpServer(std::string ip, uint16_t port)
    : ip_(std::move(ip))
    , port_(port)
{ }

TcpServer::~TcpServer() = default;

void TcpServer::start()
{
    int listenfd = create_and_listen_();

    struct epoll_event events[kMaxEvents];
    int epollfd = epoll_create1(EPOLL_CLOEXEC);
    if (epollfd < 0)
        handle_err("epoll_create1");
    
    addfd_(epollfd, listenfd);

    char buffer[kMaxBuffer];
    while (true)
    {
        int nums = epoll_wait(epollfd, events, kMaxEvents, -1);

        if (nums < 0)
            handle_err("epoll_wait");

        for (int i = 0; i < nums; ++i)
        {
            int fd = events[i].data.fd;
            
            if (fd == listenfd)
            {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int clientfd = accept4(fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
                if (clientfd < 0)
                    handle_err("accept4");
                addfd_(epollfd, clientfd);

                /* 打印新来的连接 */
                char buf[10];
                printf("new connection from [%s : %d], accept socket fd = %d\n",
                    inet_ntop(AF_INET, &client_addr.sin_addr, buf, sizeof(buf)),
                    ntohs(client_addr.sin_port),
                    clientfd);
            }
            else if (events[i].events & EPOLLIN)
            {
                memset(buffer, 0, sizeof(buffer));
                ssize_t recv_nums = recv(fd, buffer, kMaxBuffer - 1, 0);
                if (recv_nums <= 0)
                {
                    close(fd);
                    continue;
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
            else
            {
                printf("something else happened\n");
            }
        }
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

void TcpServer::addfd_(int epollfd, int fd)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) < 0)
        handle_err("epoll_ctl");    
}
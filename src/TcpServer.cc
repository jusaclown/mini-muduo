#include "src/TcpServer.h"
#include "src/common.h"
#include "src/Acceptor.h"
#include "src/TcpConnection.h"


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

TcpServer::TcpServer(EventLoop* loop)
    : TcpServer(loop, "0.0.0.0", 10086)
{}

TcpServer::TcpServer(EventLoop* loop, std::string ip, uint16_t port)
    : loop_(loop)
    , acceptor_(std::make_unique<Acceptor>(loop_, std::move(ip), port))
{
    acceptor_->set_new_connection_callback(
        std::bind(&TcpServer::handle_listenfd_, this, _1, _2)
    );
}

TcpServer::~TcpServer()
{}

void TcpServer::start()
{
    acceptor_->listen();
    loop_->loop();
}

void TcpServer::handle_listenfd_(int clientfd, struct sockaddr_in client_addr)
{
    auto conn = std::make_shared<TcpConnection>(loop_, clientfd, client_addr);
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
#include "src/TcpServer.h"
#include "src/common.h"
#include "src/Acceptor.h"
#include "src/TcpConnection.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
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
}

void TcpServer::handle_listenfd_(int clientfd, struct sockaddr_in client_addr)
{
    auto conn = std::make_shared<TcpConnection>(loop_, clientfd, client_addr);
    conn->set_message_callback(message_callback_);
    conn->set_connection_callback(connection_callback_);
    conn->set_close_callback(std::bind(&TcpServer::remove_connection_, this, _1));
    connections_[clientfd] = conn;

    conn->connect_established();
}

void TcpServer::remove_connection_(const tcp_conn_ptr& conn)
{
    connections_.erase(conn->fd());
}
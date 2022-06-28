#include "src/TcpServer.h"
#include "src/common.h"
#include "src/Acceptor.h"
#include "src/TcpConnection.h"
#include "src/EventLoopThreadPool.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <cstdio>
#include <stdlib.h>
#include <vector>

using namespace std::placeholders;

TcpServer::TcpServer(EventLoop* loop, std::string ip, uint16_t port)
    : loop_(loop)
    , acceptor_(std::make_unique<Acceptor>(loop_, std::move(ip), port))
    , thread_pool_(std::make_shared<EventLoopThreadPool>(loop))
{
    acceptor_->set_new_connection_callback(
        std::bind(&TcpServer::handle_listenfd_, this, _1, _2)
    );
}

TcpServer::~TcpServer()
{
    // TODO: close all conn in connections_
}

void TcpServer::start()
{
    thread_pool_->start(thread_init_callback_);
    loop_->run_in_loop(std::bind(&Acceptor::listen, acceptor_.get()));
}

void TcpServer::set_thread_num(int num_threads)
{
    thread_pool_->set_thread_num(num_threads);
}

void TcpServer::handle_listenfd_(int clientfd, struct sockaddr_in client_addr)
{
    loop_->assert_in_loop_thread();
    EventLoop* ioloop = thread_pool_->get_next_loop();
    
    auto conn = std::make_shared<TcpConnection>(ioloop, clientfd, client_addr);
    conn->set_message_callback(message_callback_);
    conn->set_connection_callback(connection_callback_);
    conn->set_close_callback(std::bind(&TcpServer::remove_connection_, this, _1));
    conn->set_write_complete_callback(write_complete_callback_);
    connections_[clientfd] = conn;
    
    ioloop->run_in_loop(std::bind(&TcpConnection::connect_established, conn));
}

void TcpServer::remove_connection_(const tcp_conn_ptr& conn)
{
    loop_->run_in_loop(std::bind(&TcpServer::remove_connection_in_loop_, this, conn));
}

void TcpServer::remove_connection_in_loop_(const tcp_conn_ptr& conn)
{
    loop_->assert_in_loop_thread();
    size_t n = connections_.erase(conn->fd());
    (void)n;
    assert(n == 1);
}
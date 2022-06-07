#pragma once

#include "src/Channel.h"
#include "src/common.h"

#include <string>
#include <map>
#include <memory>

class Acceptor;

class TcpServer : public std::enable_shared_from_this<TcpServer>
{
public:
    TcpServer(EventLoop* loop);
    TcpServer(EventLoop* loop, std::string ip, uint16_t port);
    ~TcpServer();

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    void start();
   
private:
    void handle_listenfd_(int clientfd, struct sockaddr_in client_addr);    /* 处理监听套接字可读事件的回调函数，通常表示有新连接到来 */
    void handle_clientfd_(tcp_conn_ptr conn, char* buf, ssize_t n);         /* 处理其他套接字可读事件的回调函数，这里只是回显 */
    void remove_connection_(tcp_conn_ptr conn);
    
private:
    EventLoop* loop_;
    std::unique_ptr<Acceptor> acceptor_;
    connection_map connections_;        /* 用来保存新到的连接 */
};
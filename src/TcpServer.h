#pragma once

#include "src/Channel.h"
#include "src/common.h"

#include <string>
#include <vector>
#include <map>
#include <memory>

class Acceptor;

class TcpServer : public std::enable_shared_from_this<TcpServer>
{
public:
    TcpServer();
    TcpServer(std::string ip, uint16_t port);
    ~TcpServer();

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    void start();
    void update_channel(Channel* Channel);
    
private:
    void handle_listenfd_(int clientfd, struct sockaddr_in client_addr);    /* 处理监听套接字可读事件的回调函数，通常表示有新连接到来 */
    void handle_clientfd_(tcp_conn_ptr conn, char* buf, ssize_t n);          /* 处理其他套接字可读事件的回调函数，这里只是回显 */
    void remove_connection_(tcp_conn_ptr conn);
    
private:
    using event_list = std::vector<struct epoll_event>;
    // using channel_map = std::map<int, Channel*>;
    using connection_map = std::map<int, tcp_conn_ptr>;
    
    static const int kMaxEvents = 100;

    std::unique_ptr<Acceptor> acceptor_;
    int epollfd_;                       /* epoll_create的文件描述符 */
    event_list events_;                 /* epoll_wait填充的epoll_event数组 */
    // channel_map channels_;           /* 用来保存新到的连接，并在关闭时释放内存 */
    connection_map connections_;        /* 用来保存新到的连接 */
};
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
    void set_message_callback(message_callback cb)
    {
        message_callback_ = std::move(cb);
    }

    void set_connection_callback(connection_callback cb)
    {
        connection_callback_ = std::move(cb);
    }

    void set_write_complete_callback(write_complete_callback cb)
    {
        write_complete_callback_ = std::move(cb);
    }

private:
    void handle_listenfd_(int clientfd, struct sockaddr_in client_addr);    /* 处理监听套接字可读事件的回调函数，通常表示有新连接到来 */
    void remove_connection_(const tcp_conn_ptr& conn);

private:
    EventLoop* loop_;
    std::unique_ptr<Acceptor> acceptor_;
    connection_map connections_;        /* 用来保存新到的连接 */
    
    message_callback message_callback_;
    connection_callback connection_callback_;
    write_complete_callback write_complete_callback_;
};
#pragma once

#include "src/EventLoop.h"
#include "src/Channel.h"

class Acceptor
{
public:
    using new_connection_callback = std::function<void(int sockfd, struct sockaddr_in addr)>;
    
    Acceptor(EventLoop* loop, std::string ip, uint16_t port);
    ~Acceptor();

    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;

    void listen();
    void set_new_connection_callback(new_connection_callback cb)
    {
        new_connection_callback_ = std::move(cb);
    }

private:
    int create_and_bind_(std::string ip, uint16_t port);   /* socket -> bind */
    void handle_read_();         /* 处理监听套接字可读事件的回调函数，通常表示有新连接到来 */

private:
    static const int kMaxListen = 10;
    
    EventLoop* loop_;           /* 所属EventLoop */
    int listenfd_;              /* 监听套接字对应的fd */
    Channel listen_channel_;    /* 监听套接字对应的Channel */
    int idlefd_;                /* 用于防止fd达到上限新的用户无法连接的情况 */
    new_connection_callback new_connection_callback_;   /* 新连接到来 如何处理 由TcpServer提供*/
};
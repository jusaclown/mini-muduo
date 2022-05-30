#pragma once

#include "src/Channel.h"

#include <string>
#include <vector>
#include <map>
#include <memory>

class TcpServer : public std::enable_shared_from_this<TcpServer>
{
public:
    TcpServer();
    TcpServer(std::string ip, uint16_t port);
    ~TcpServer();

    void start();
    void update_channel(Channel* Channel);
    
private:
    int create_and_listen_();           /* socket -> bind -> listen */
    void handle_listenfd_();            /* 处理监听套接字可读事件的回调函数，通常表示有新连接到来 */
    void handle_clientfd_(int fd);      /* 处理其他套接字可读事件的回调函数，这里只是回显 */

private:
    using event_list = std::vector<struct epoll_event>;
    using channel_map = std::map<int, Channel*>;
    
    static const int kMaxListen = 10;
    static const int kMaxEvents = 100;
    static const size_t kMaxBuffer = 1024;

    const std::string ip_;      /* 服务器绑定的ip地址 */
    uint16_t port_;             /* 服务器绑定的端口号 */

    int epollfd_;               /* epoll_create的文件描述符 */
    Channel listen_channel_;    /* 监听套接字对应的Channel */
    event_list events_;         /* epoll_wait填充的epoll_event数组 */
    channel_map channels_;      /* 用来保存新到的连接，并在关闭时释放内存 */
};
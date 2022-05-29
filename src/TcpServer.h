#pragma once

#include <string>

class TcpServer
{
public:
    TcpServer();
    TcpServer(std::string ip, uint16_t port);
    ~TcpServer();

    void start();

private:
    int create_and_listen_();
    void addfd_(int epollfd, int fd);

private:
    static const int kMaxListen = 10;
    static const int kMaxEvents = 100;
    static const size_t kMaxBuffer = 1024;

    const std::string ip_;
    uint16_t port_;
};
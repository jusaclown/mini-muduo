#include "src/Acceptor.h"
#include "src/common.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

Acceptor::Acceptor(EventLoop* loop, std::string ip, uint16_t port)
    : loop_(loop)
    , listenfd_(create_and_bind_(std::move(ip), port))
    , listen_channel_(loop_, listenfd_)
    , idlefd_(::open("dev/null", O_RDONLY | O_CLOEXEC))
{
    listen_channel_.set_read_callback(std::bind(&Acceptor::handle_read_, this));
}

Acceptor::~Acceptor()
{
    ::close(listenfd_);
    ::close(idlefd_);
}

void Acceptor::listen()
{
    int ret = ::listen(listenfd_, kMaxListen);
    if (ret < 0)
        handle_err("listen");
    
    listen_channel_.enable_reading();
}

int Acceptor::create_and_bind_(std::string ip, uint16_t port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (sockfd < 0)
        handle_err("socket()");

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = static_cast<in_port_t>(htons(port));
    inet_pton(AF_INET, ip.c_str(), &server_address.sin_addr);
    // server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    int ret = ::bind(sockfd, reinterpret_cast<struct sockaddr*>(&server_address), sizeof(server_address));
    if (ret < 0)
        handle_err("bind()");
    
    return sockfd;
}

void Acceptor::handle_read_()
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = static_cast<socklen_t>(sizeof(client_addr));
    
    int clientfd = accept4(listenfd_, reinterpret_cast<struct sockaddr*>(&client_addr),
        &client_addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (clientfd < 0)
    {
        if (errno == EMFILE)
        {
            ::close(idlefd_);
            idlefd_ = ::accept(listenfd_, nullptr, nullptr);
            ::close(idlefd_);
            ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
        else
            handle_err("accept4");
    }

    if (new_connection_callback_)
        new_connection_callback_(clientfd, client_addr);
    else
        ::close(clientfd);
}
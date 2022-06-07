#include "src/EventLoop.h"
#include "src/TcpServer.h"
#include "src/common.h"
#include "src/TcpConnection.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void on_connection(const tcp_conn_ptr& conn)
{
    char buf[10];
    auto client_addr = conn->peer_addr();
    printf("new connection from [%s : %d], accept socket fd = %d\n",
        inet_ntop(AF_INET, &client_addr.sin_addr, buf, sizeof(buf)),
        ntohs(client_addr.sin_port),
        conn->fd());
}

void send_message(const tcp_conn_ptr& conn, char* buffer, ssize_t recv_nums)
{
    ssize_t send_num = send(conn->fd(), buffer, recv_nums, 0);
    if (send_num != recv_nums)
    {
        printf("There was a mistake(send_num != recv_nums), but we decided to continue\n");
    }
}

int main(int argc, char* argv[])
{
    EventLoop loop;
    TcpServer tcp_server(&loop);
    tcp_server.set_connection_callback(on_connection);
    tcp_server.set_message_callback(send_message);
    tcp_server.start();
    loop.loop();

    return EXIT_SUCCESS;
}
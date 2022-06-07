#include "src/EventLoop.h"
#include "src/TcpServer.h"

int main(int argc, char* argv[])
{
    EventLoop loop;
    TcpServer tcp_server(&loop);
    tcp_server.start();
    
    return EXIT_SUCCESS;
}
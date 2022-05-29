#include "src/TcpServer.h"

int main(int argc, char* argv[])
{
    TcpServer tcp_server;
    tcp_server.start();
    
    return EXIT_SUCCESS;
}
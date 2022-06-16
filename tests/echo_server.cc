#include "src/EventLoop.h"
#include "src/TcpServer.h"
#include "src/common.h"
#include "src/TcpConnection.h"
#include "src/Buffer.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <functional>
#include <list>

using namespace std::placeholders;

class EchoServer
{
public:
    EchoServer(EventLoop* loop, std::string ip, uint16_t port, int idle_seconds)
        : tcp_server_(loop, std::move(ip), port)
        , idle_seconds_(idle_seconds)
    {
        printf("EchoServer()\n");
        tcp_server_.set_connection_callback(std::bind(&EchoServer::on_connection, this, _1));
        tcp_server_.set_message_callback(std::bind(&EchoServer::send_message, this, _1, _2));
        loop->run_every(std::chrono::seconds(1), std::bind(&EchoServer::on_timer, this));
        dump_connection_list();
    }

    void start()
    {
        tcp_server_.start();
    }

private:
    void on_connection(const tcp_conn_ptr& conn)
    {
        printf("on_connection()\n");
        char buf[10];
        auto client_addr = conn->peer_addr();
        printf("connection from [%s : %d], socket fd = %d is %s\n",
            inet_ntop(AF_INET, &client_addr.sin_addr, buf, sizeof(buf)),
            ntohs(client_addr.sin_port),
            conn->fd(),
            conn->connected() ? "up" : "down");

        if (conn->connected())
        {
            Node node;
            node.last_receive_time = timer_clock::now();
            connection_list_.push_back(conn);
            node.position = --connection_list_.end();
            conn->set_context(node);
        }
        else
        {
            const Node& n = std::any_cast<const Node&>(conn->get_context());
            connection_list_.erase(n.position);
        }

        dump_connection_list();
    }

    void send_message(const tcp_conn_ptr& conn, Buffer& buffer)
    {
        printf("send_message() by %d\n", conn->fd());
        std::string msg = buffer.retrieve_all_as_string();
        conn->send(msg);

        Node* node = std::any_cast<Node>(conn->get_mutable_context());
        node->last_receive_time = timer_clock::now();
        connection_list_.splice(connection_list_.end(), connection_list_, node->position);

        dump_connection_list();
    }

    void on_timer()
    {
        printf("on_timer()\n");
        timer_clock::time_point now = timer_clock::now();
        for (auto it = connection_list_.begin(); it != connection_list_.end();)
        {
            tcp_conn_ptr conn = it->lock();
            if (conn)
            {
                Node* n = std::any_cast<Node>(conn->get_mutable_context());
                auto age = now - n->last_receive_time;
                if (age > idle_seconds_)    // 超时
                {
                    if (conn->connected())
                    {
                        printf("%d long time no data, shutdown it\n", conn->fd());
                        conn->shutdown();
                    }
                }
                else
                {
                    break;
                }
                ++it;
            }
            else
            {
                it = connection_list_.erase(it);    // 删除了之后node.position会不会受到影响
            }
        }
        dump_connection_list();
    }

    void dump_connection_list() const
    {
        printf("    size = %ld\n", connection_list_.size());
        for (auto it = connection_list_.cbegin(); it != connection_list_.cend(); ++it)
        {
            tcp_conn_ptr conn = it->lock();
            if (conn)
            {
                printf("    conn fd = %d ", conn->fd());
                const Node& n = std::any_cast<const Node&>(conn->get_context());
                printf(" time %ld\n", n.last_receive_time.time_since_epoch().count());
            }
            else
            {
                 printf("    expired\n");
            }
        }
    }

private:
    using weak_conn_ptr = std::weak_ptr<TcpConnection>;
    using weak_conn_list = std::list<weak_conn_ptr>;

    struct Node
    {
        timer_clock::time_point last_receive_time;
        weak_conn_list::iterator position;
    };

    TcpServer tcp_server_;
    std::chrono::seconds idle_seconds_;
    weak_conn_list connection_list_;
};


int main(int argc, char* argv[])
{
    printf("Echo Server is running...\n");
    EventLoop loop;
    EchoServer echo_server(&loop, "0.0.0.0", 10086, 5);
    echo_server.start();
    loop.loop();
    
    return EXIT_SUCCESS;
}
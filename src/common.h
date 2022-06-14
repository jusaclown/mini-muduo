#pragma once

#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <chrono>

#define handle_err(msg) do { perror(msg); exit(EXIT_FAILURE);} while(0);

class Channel;
class TcpConnection;
class Buffer;

using tcp_conn_ptr = std::shared_ptr<TcpConnection>;
using message_callback = std::function<void(const tcp_conn_ptr&, Buffer&)>;
using connection_callback = std::function<void(const tcp_conn_ptr&)>;
using close_callback = std::function<void(const tcp_conn_ptr&)>;
using write_complete_callback = std::function<void(const tcp_conn_ptr&)>;
using timer_callback = std::function<void()>;

using event_list = std::vector<struct epoll_event>;
using channel_map = std::map<int, Channel*>;
using channel_list = std::vector<Channel*>;
using connection_map = std::map<int, tcp_conn_ptr>;

using timer_clock = std::chrono::steady_clock;
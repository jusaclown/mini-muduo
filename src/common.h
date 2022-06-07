#pragma once

#include <vector>
#include <map>
#include <memory>
#include <functional>

#define handle_err(msg) do { perror(msg); exit(EXIT_FAILURE);} while(0);

class Channel;
class TcpConnection;

using tcp_conn_ptr = std::shared_ptr<TcpConnection>;
using message_callback = std::function<void(tcp_conn_ptr, char*, ssize_t)>;
using close_callback = std::function<void(tcp_conn_ptr)>;

using event_list = std::vector<struct epoll_event>;
using channel_map = std::map<int, Channel*>;
using channel_list = std::vector<Channel*>;
using connection_map = std::map<int, tcp_conn_ptr>;
#pragma once

#define handle_err(msg) do { perror(msg); exit(EXIT_FAILURE);} while(0);

class TcpConnection;

using tcp_conn_ptr = std::shared_ptr<TcpConnection>;
using message_callback = std::function<void(tcp_conn_ptr, char*, ssize_t)>;
using close_callback = std::function<void(tcp_conn_ptr)>;
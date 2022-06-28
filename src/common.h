#pragma once

#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <chrono>
#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */

#define handle_err(msg) do { perror(msg); exit(EXIT_FAILURE);} while(0);

class Channel;
class TcpConnection;
class Buffer;
class Timer;
class EventLoop;

using tcp_conn_ptr = std::shared_ptr<TcpConnection>;
using message_callback = std::function<void(const tcp_conn_ptr&, Buffer&)>;
using connection_callback = std::function<void(const tcp_conn_ptr&)>;
using close_callback = std::function<void(const tcp_conn_ptr&)>;
using write_complete_callback = std::function<void(const tcp_conn_ptr&)>;
using high_water_mark_callback = std::function<void(const tcp_conn_ptr&, size_t)>;
using timer_callback = std::function<void()>;
using thread_init_callback = std::function<void(EventLoop*)>;

using event_list = std::vector<struct epoll_event>;
using channel_map = std::map<int, Channel*>;
using channel_list = std::vector<Channel*>;
using connection_map = std::map<int, tcp_conn_ptr>;

using timer_clock = std::chrono::steady_clock;
using timer_ptr = std::shared_ptr<Timer>;
using TimerId = timer_ptr;

inline std::size_t thread_id() noexcept
{
    static thread_local const std::size_t tid = static_cast<std::size_t>(::syscall(SYS_gettid));
    return tid;
}
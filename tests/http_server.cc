#include "src/common.h"
#include "src/Buffer.h"
#include "src/EventLoop.h"
#include "src/TcpServer.h"
#include "src/TcpConnection.h"

#include <string>
#include <map>
#include <cassert>
#include <iostream>
#include <functional>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

enum http_version
{
    kUnknow, kHttp10, kHttp11
};

std::string version_string(http_version v)
{
    switch (v)
    {
    case kHttp10:
        return "HTTP/1.0";
    case kHttp11:
        return "HTTP/1.1";
    default:
        return "UNKNOW";
    }
}

class HttpRequest
{
public:
    enum http_method
    {
        kInvalid, kGet, kPost, kHead, kPut, kDelete
    };

    HttpRequest()
        : method_(kInvalid)
        , version_(kUnknow)
    {}

    HttpRequest(const HttpRequest&) = default;
    HttpRequest& operator=(const HttpRequest&) = default;
    // move
    HttpRequest(HttpRequest&& rhs)
        : method_(rhs.method_)
        , version_(rhs.version_)
        , path_(std::move(rhs.path_))
        , query_(std::move(rhs.query_))
        , receive_time_(rhs.receive_time_)
        , headers_(std::move(rhs.headers_))
    {}
    
    HttpRequest& operator=(HttpRequest&& rhs)
    {
        if (&rhs != this)
        {
            method_ = rhs.method_;
            version_ = rhs.version_;
            path_ = std::move(rhs.path_);
            query_ = std::move(rhs.query_);
            receive_time_ = rhs.receive_time_;
            headers_ = std::move(rhs.headers_);
        }
        
        return *this;
    }

    void set_version(http_version v) { version_ = v; }
    http_version version() const { return version_; }
    
    http_method method() const { return method_; }
    bool set_method(const char* start, const char* end)
    {
        assert(method_ == kInvalid);
        std::string m(start, end);
        if (m == "GET") { method_ = kGet; }
        else if (m == "POST") { method_ = kPost; }
        else if (m == "HEAD") { method_ = kHead; }
        else if (m == "PUT") { method_ = kPut; }
        else if (m == "DELETE") { method_ = kDelete; }
        else { method_ = kInvalid; }
        return method_ != kInvalid;
    }

    std::string method_string() const
    {
        switch (method_)
        {
        case kGet:
            return "GET";
        case kPost:
            return "POST";
        case kHead:
            return "HEAD";
        case kPut:
            return "PUT";
        case kDelete:
            return "DELETE";
        default:
            return "UNKNOW";
        }
    }

    void set_path(const char* start, const char* end) { path_.assign(start, end);; }
    const std::string& path() const { return path_; }

    void set_query(const char* start, const char* end) { query_.assign(start, end); }
    const std::string& query() const { return query_; }

    void set_receive_time(timer_clock::time_point tp) { receive_time_ = tp; }
    timer_clock::time_point receive_time() const { return receive_time_; }
    
    void add_header(const char* start, const char* colon, const char* end)
    {
        std::string key(start, colon);
        ++colon;
        while (colon < end && isspace(*colon))
        {
            ++colon;
        }
        std::string value(colon, end);
        while (!value.empty() && isspace(value[value.size()-1]))
        {
            value.resize(value.size() - 1);
        }
        headers_[std::move(key)] = std::move(value);
    }

    std::string get_header(const std::string& key) const
    {
        std::string result;
        if (headers_.count(key))
        {
            result = headers_.at(key);
        }
        return result;
    }

    const std::map<std::string, std::string>&
    headers() const { return headers_; }

    void swap(HttpRequest& other)
    {
        std::swap(method_, other.method_);
        std::swap(version_, other.version_);
        path_.swap(other.path_);
        query_.swap(other.query_);
        std::swap(receive_time_, other.receive_time_);
        headers_.swap(other.headers_);
    }

private:
    http_method method_;
    http_version version_;
    std::string path_;
    std::string query_;
    timer_clock::time_point receive_time_;
    std::map<std::string, std::string> headers_;
};


class HttpResponse
{
public:
    enum http_status_code
    {
        kUnknow,
        k200Ok = 200,
        k201MovedPerrmanently = 301,
        k200BadRequest = 400,
        k404NotFound = 404
    };
    
    explicit HttpResponse(bool close)
        : version_(http_version::kUnknow)
        , status_code_(kUnknow)
        , close_connection_(close)
    {}

    HttpResponse(const HttpResponse&) = default;
    HttpResponse& operator=(const HttpResponse&) = default;
    
    HttpResponse(HttpResponse&& rhs)
        : version_(rhs.version_)
        , status_code_(rhs.status_code_)
        , status_message_(std::move(rhs.status_message_))
        , close_connection_(rhs.close_connection_)
        , body_(std::move(rhs.body_))
        , headers_(std::move(rhs.headers_))
    {}

    HttpResponse& operator=(HttpResponse&& rhs)
    {
        if (&rhs != this)
        {
            version_ = rhs.version_;
            status_code_ = rhs.status_code_;
            status_message_ = std::move(rhs.status_message_);
            close_connection_ = rhs.close_connection_;
            body_ = std::move(rhs.body_);
            headers_ = std::move(rhs.headers_);
        }
        return *this;
    }

    void set_status_code(http_status_code code) { status_code_ = code; }
    http_status_code status_code() const { return status_code_; }

    void set_version(http_version v) { version_ = v; }
    http_version version() const { return version_; }

    void set_status_message(std::string message) { status_message_ = std::move(message); }
    const std::string& status_message() const { return status_message_; }

    void set_close_connection(bool on) { close_connection_ = on; }
    bool close_connection() const { return close_connection_; }

    void set_content_type(std::string content_type)
    {
        add_header("Content-Type", std::move(content_type));
    }

    void add_header(std::string key, std::string value)
    {
        headers_[std::move(key)] = std::move(value);
    }

    void set_body(std::string body)
    {
        body_ = std::move(body);
    }

    void append_buffer(Buffer& output) const
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%s %d ", version_string(version_).data(), status_code_);
        output.append(buf);
        output.append(status_message_);
        output.append("\r\n");

        if (close_connection_)
        {
            output.append("Connection: close\r\n");
        }
        else
        {
            snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
            output.append(buf);
            output.append("Connection: Keep-Alive\r\n");
        }

        for (const auto& header : headers_)
        {
            output.append(header.first);
            output.append(": ");
            output.append(header.second);
            output.append("\r\n");    
        }

        output.append("\r\n");
        output.append(body_);
    }

private:
    http_version version_;
    http_status_code status_code_;
    std::string status_message_;
    bool close_connection_;
    std::string body_;
    std::map<std::string, std::string> headers_;
};


class HttpContext
{
public:
    enum http_request_parse_state
    {
        kExpectRequestLine,
        kExpectHeaders,
        kExpectBody,
        kGotAll,
    };

    HttpContext()
        : state_(kExpectRequestLine)
    {}

    bool gotall() const { return state_ == kGotAll; }

    void reset()
    {
        state_ = kExpectRequestLine;
        HttpRequest dummy;
        request_.swap(dummy);
    }

    const HttpRequest& request() const { return request_; }
    HttpRequest& request() { return request_; }

    // return false if any error
    bool parse_request(Buffer& buf, timer_clock::time_point receive_time)
    {
        bool ok = true;
        bool has_more = true;

        while (has_more)
        {
            if (state_ == kExpectRequestLine)
            {
                const char* crlf = buf.find_crlf();
                if (crlf)
                {
                    ok = process_request_line(buf.peek(), crlf);
                    if (ok)
                    {
                        request_.set_receive_time(receive_time);
                        buf.retrieve_until(crlf + 2);
                        state_ = kExpectHeaders;
                    }
                    else
                    {
                        has_more = false;
                    }
                }
                else
                {
                    has_more = false;
                }
            }
            else if (state_ == kExpectHeaders)
            {
                // 解析头部信息
                const char* crlf = buf.find_crlf();
                if (crlf)
                {
                    const char* colon = std::find(buf.peek(), crlf, ':');
                    if (colon != crlf)
                    {
                        request_.add_header(buf.peek(), colon, crlf);
                    }
                    else    
                    {
                        // 空行
                        state_ = kExpectBody;
                    }
                    buf.retrieve_until(crlf + 2);
                }
                else
                {
                    has_more = false;
                }
            }
            else if (state_ == kExpectBody)
            {
                // TODO 处理BODY信息
                state_ = kGotAll;
            }
            else if (state_ == kGotAll)
            {
                return true;
            }
            else
            {
                // printf("Unkonw state!");
                return false;
            }
        }
        
        return ok;
    }

private:
    // 解析请求行 "GET /AAA/BBB HTTP/1.1"
    bool process_request_line(const char* begin, const char* end)
    {
        bool succeed = false;
        const char* start = begin;
        const char* space = std::find(start, end, ' ');
        if (space != end && request_.set_method(start, space))
        {
            start = space + 1;
            space = std::find(start, end, ' ');
            if (space != end)
            {
                const char* question = std::find(start, space, '?');
                if (question != space)
                {
                    request_.set_path(start, question);
                    request_.set_query(question, space);
                }
                else
                {
                    request_.set_path(start, space);
                }

                start = space + 1;
                succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
                if (succeed)
                {
                    if (*(end - 1) == '1')
                    {
                        request_.set_version(kHttp11);
                    }
                    else if (*(end - 1) == '0')
                    {
                        request_.set_version(kHttp10);
                    }
                    else
                    {
                        succeed = false;
                    }
                }
            }
        }
        return succeed;
    }

private:
    http_request_parse_state state_;
    HttpRequest request_;
};


void default_http_callback(const HttpRequest&, HttpResponse& resp)
{
    resp.set_version(kHttp11);
    resp.set_status_code(HttpResponse::k404NotFound);
    resp.set_status_message("Not Found");
    resp.set_close_connection(true);
}

using namespace std::placeholders;
class HttpServer
{
public:
    using http_callback = std::function<void(const HttpRequest&, HttpResponse&)>;

    HttpServer(EventLoop* loop, std::string ip, uint16_t port)
        : server_(loop, std::move(ip), port)
        , http_callback_(default_http_callback)
    {
        server_.set_connection_callback(std::bind(&HttpServer::on_connection, this, _1));
        server_.set_message_callback(std::bind(&HttpServer::on_message, this, _1, _2));
    }

    void set_http_callback(const http_callback& cb)
    {
        http_callback_ = cb;
    }

    void start()
    {
        server_.start();
    }
    
    void set_thread_num(int num_threads)
    {
        server_.set_thread_num(num_threads);
    }
    
private:
    void on_connection(const tcp_conn_ptr& conn)
    {
        // char buf[10];
        // auto client_addr = conn->peer_addr();
        // printf("connection from [%s : %d], socket fd = %d is %s\n",
        //     inet_ntop(AF_INET, &client_addr.sin_addr, buf, sizeof(buf)),
        //     ntohs(client_addr.sin_port),
        //     conn->fd(),
        //     conn->connected() ? "up" : "down");
        
        if (conn->connected())
        {
            conn->set_context(HttpContext());
        }
    }

    void on_message(const tcp_conn_ptr& conn, Buffer& buffer)
    {
        HttpContext* context = std::any_cast<HttpContext>(conn->get_mutable_context());
        if (!context->parse_request(buffer, timer_clock::now()))
        {
            conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
            conn->shutdown();
        }

        if (context->gotall())
        {
            on_request(conn, context->request());
            context->reset();
        }
    }

    void on_request(const tcp_conn_ptr& conn, const HttpRequest& req)
    {
        const std::string& connection = req.get_header("Connection");
        bool close = connection == "close" ||
            (req.version() == kHttp10 && connection != "Keep-Alive");
        HttpResponse response(close);
        http_callback_(req, response);
        Buffer buf;
        response.append_buffer(buf);
        conn->send(&buf);   // TODO: 直接发送，别额外复制
        if (response.close_connection())
        {
            conn->shutdown();
        }
    }

private:
    TcpServer server_;
    http_callback http_callback_;
};

extern char favicon[555];
void on_request(const HttpRequest& req, HttpResponse& resp)
{
    if (req.path() == "/")
    {
        resp.set_version(kHttp11);
        resp.set_status_code(HttpResponse::k200Ok);
        resp.set_status_message("OK");
        resp.set_content_type("text/html");
        resp.add_header("Server", "Muduo");
        resp.set_body("<html><head><title>This is title</title></head>"
            "<body><h1>Hello</h1>Hello World!\n</body></html>");
    }
    else if (req.path() == "/favicon.ico")
    {
        resp.set_version(kHttp11);
        resp.set_status_code(HttpResponse::k200Ok);
        resp.set_status_message("OK");
        resp.set_content_type("image/png");
        resp.add_header("Server", "Muduo");
        resp.set_body(std::string(favicon, sizeof(favicon)));
    }
    else
    {
        resp.set_version(kHttp11);
        resp.set_status_code(HttpResponse::k404NotFound);
        resp.set_status_message("Not Found");
        resp.set_close_connection(true);
    }
}

int main()
{
    printf("HTTP Server is running...\n");
    EventLoop loop;
    HttpServer server(&loop, "0.0.0.0", 10087);
    server.set_http_callback(on_request);
    server.set_thread_num(0);
    server.start();
    // loop.run_after(std::chrono::seconds(15), [&]() {loop.quit();});
    loop.loop();
    return 0;
}

char favicon[555] = {
  '\x89', 'P', 'N', 'G', '\xD', '\xA', '\x1A', '\xA',
  '\x0', '\x0', '\x0', '\xD', 'I', 'H', 'D', 'R',
  '\x0', '\x0', '\x0', '\x10', '\x0', '\x0', '\x0', '\x10',
  '\x8', '\x6', '\x0', '\x0', '\x0', '\x1F', '\xF3', '\xFF',
  'a', '\x0', '\x0', '\x0', '\x19', 't', 'E', 'X',
  't', 'S', 'o', 'f', 't', 'w', 'a', 'r',
  'e', '\x0', 'A', 'd', 'o', 'b', 'e', '\x20',
  'I', 'm', 'a', 'g', 'e', 'R', 'e', 'a',
  'd', 'y', 'q', '\xC9', 'e', '\x3C', '\x0', '\x0',
  '\x1', '\xCD', 'I', 'D', 'A', 'T', 'x', '\xDA',
  '\x94', '\x93', '9', 'H', '\x3', 'A', '\x14', '\x86',
  '\xFF', '\x5D', 'b', '\xA7', '\x4', 'R', '\xC4', 'm',
  '\x22', '\x1E', '\xA0', 'F', '\x24', '\x8', '\x16', '\x16',
  'v', '\xA', '6', '\xBA', 'J', '\x9A', '\x80', '\x8',
  'A', '\xB4', 'q', '\x85', 'X', '\x89', 'G', '\xB0',
  'I', '\xA9', 'Q', '\x24', '\xCD', '\xA6', '\x8', '\xA4',
  'H', 'c', '\x91', 'B', '\xB', '\xAF', 'V', '\xC1',
  'F', '\xB4', '\x15', '\xCF', '\x22', 'X', '\x98', '\xB',
  'T', 'H', '\x8A', 'd', '\x93', '\x8D', '\xFB', 'F',
  'g', '\xC9', '\x1A', '\x14', '\x7D', '\xF0', 'f', 'v',
  'f', '\xDF', '\x7C', '\xEF', '\xE7', 'g', 'F', '\xA8',
  '\xD5', 'j', 'H', '\x24', '\x12', '\x2A', '\x0', '\x5',
  '\xBF', 'G', '\xD4', '\xEF', '\xF7', '\x2F', '6', '\xEC',
  '\x12', '\x20', '\x1E', '\x8F', '\xD7', '\xAA', '\xD5', '\xEA',
  '\xAF', 'I', '5', 'F', '\xAA', 'T', '\x5F', '\x9F',
  '\x22', 'A', '\x2A', '\x95', '\xA', '\x83', '\xE5', 'r',
  '9', 'd', '\xB3', 'Y', '\x96', '\x99', 'L', '\x6',
  '\xE9', 't', '\x9A', '\x25', '\x85', '\x2C', '\xCB', 'T',
  '\xA7', '\xC4', 'b', '1', '\xB5', '\x5E', '\x0', '\x3',
  'h', '\x9A', '\xC6', '\x16', '\x82', '\x20', 'X', 'R',
  '\x14', 'E', '6', 'S', '\x94', '\xCB', 'e', 'x',
  '\xBD', '\x5E', '\xAA', 'U', 'T', '\x23', 'L', '\xC0',
  '\xE0', '\xE2', '\xC1', '\x8F', '\x0', '\x9E', '\xBC', '\x9',
  'A', '\x7C', '\x3E', '\x1F', '\x83', 'D', '\x22', '\x11',
  '\xD5', 'T', '\x40', '\x3F', '8', '\x80', 'w', '\xE5',
  '3', '\x7', '\xB8', '\x5C', '\x2E', 'H', '\x92', '\x4',
  '\x87', '\xC3', '\x81', '\x40', '\x20', '\x40', 'g', '\x98',
  '\xE9', '6', '\x1A', '\xA6', 'g', '\x15', '\x4', '\xE3',
  '\xD7', '\xC8', '\xBD', '\x15', '\xE1', 'i', '\xB7', 'C',
  '\xAB', '\xEA', 'x', '\x2F', 'j', 'X', '\x92', '\xBB',
  '\x18', '\x20', '\x9F', '\xCF', '3', '\xC3', '\xB8', '\xE9',
  'N', '\xA7', '\xD3', 'l', 'J', '\x0', 'i', '6',
  '\x7C', '\x8E', '\xE1', '\xFE', 'V', '\x84', '\xE7', '\x3C',
  '\x9F', 'r', '\x2B', '\x3A', 'B', '\x7B', '7', 'f',
  'w', '\xAE', '\x8E', '\xE', '\xF3', '\xBD', 'R', '\xA9',
  'd', '\x2', 'B', '\xAF', '\x85', '2', 'f', 'F',
  '\xBA', '\xC', '\xD9', '\x9F', '\x1D', '\x9A', 'l', '\x22',
  '\xE6', '\xC7', '\x3A', '\x2C', '\x80', '\xEF', '\xC1', '\x15',
  '\x90', '\x7', '\x93', '\xA2', '\x28', '\xA0', 'S', 'j',
  '\xB1', '\xB8', '\xDF', '\x29', '5', 'C', '\xE', '\x3F',
  'X', '\xFC', '\x98', '\xDA', 'y', 'j', 'P', '\x40',
  '\x0', '\x87', '\xAE', '\x1B', '\x17', 'B', '\xB4', '\x3A',
  '\x3F', '\xBE', 'y', '\xC7', '\xA', '\x26', '\xB6', '\xEE',
  '\xD9', '\x9A', '\x60', '\x14', '\x93', '\xDB', '\x8F', '\xD',
  '\xA', '\x2E', '\xE9', '\x23', '\x95', '\x29', 'X', '\x0',
  '\x27', '\xEB', 'n', 'V', 'p', '\xBC', '\xD6', '\xCB',
  '\xD6', 'G', '\xAB', '\x3D', 'l', '\x7D', '\xB8', '\xD2',
  '\xDD', '\xA0', '\x60', '\x83', '\xBA', '\xEF', '\x5F', '\xA4',
  '\xEA', '\xCC', '\x2', 'N', '\xAE', '\x5E', 'p', '\x1A',
  '\xEC', '\xB3', '\x40', '9', '\xAC', '\xFE', '\xF2', '\x91',
  '\x89', 'g', '\x91', '\x85', '\x21', '\xA8', '\x87', '\xB7',
  'X', '\x7E', '\x7E', '\x85', '\xBB', '\xCD', 'N', 'N',
  'b', 't', '\x40', '\xFA', '\x93', '\x89', '\xEC', '\x1E',
  '\xEC', '\x86', '\x2', 'H', '\x26', '\x93', '\xD0', 'u',
  '\x1D', '\x7F', '\x9', '2', '\x95', '\xBF', '\x1F', '\xDB',
  '\xD7', 'c', '\x8A', '\x1A', '\xF7', '\x5C', '\xC1', '\xFF',
  '\x22', 'J', '\xC3', '\x87', '\x0', '\x3', '\x0', 'K',
  '\xBB', '\xF8', '\xD6', '\x2A', 'v', '\x98', 'I', '\x0',
  '\x0', '\x0', '\x0', 'I', 'E', 'N', 'D', '\xAE',
  'B', '\x60', '\x82',
};

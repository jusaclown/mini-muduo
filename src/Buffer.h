#pragma once

#include <vector>
#include <cassert>
#include <string>
#include <algorithm>

/** A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
 *
 *  @code
 *  +-------------------+------------------+------------------+
 *  | prependable bytes |  readable bytes  |  writable bytes  |
 *  |                   |     (CONTENT)    |                  |
 *  +-------------------+------------------+------------------+
 *  |                   |                  |                  |
 *  0      <=      readerIndex   <=   writerIndex    <=     size
 *  @endcode 
 */
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initial_size = kInitialSize);

    Buffer(const Buffer&) = default;
    Buffer& operator=(const Buffer&) = default;

    Buffer(Buffer&& buffer);
    Buffer& operator=(Buffer&& buffer);

    void swap(Buffer& rhs);

    size_t readable_bytes() const { return writer_index_ - reader_index_; }
    size_t writable_bytes() const { return buffer_.size() - writer_index_; }
    size_t prependable_bytes() const { return reader_index_; }

    /* 可读数据开始的地方 */
    const char* peek() const
    {
        return begin() + reader_index_;
    }

    /* 回收len长度的空间 */
    void retrieve(size_t len)
    {
        assert(len <= readable_bytes());
        if (len < readable_bytes())
        {
            reader_index_ += len;
        }
        else
        {
            retrieve_all();
        }
    }

    void retrieve_until(const char* end)
    {
        assert(peek() <= end);
        assert(end <= begin_write());
        retrieve(end - peek());
    }

    void retrieve_all()
    {
        reader_index_ = kCheapPrepend;
        writer_index_ = kCheapPrepend;
    }

    char* begin_write()
    {
        return begin() + writer_index_;
    }

    const char* begin_write() const
    {
        return begin() + writer_index_;
    }

    void has_written(size_t len)
    {
        assert(len <= writable_bytes());
        writer_index_ += len;
    }

    void unwrite(size_t len)
    {
        assert(len <= readable_bytes());
        writer_index_ -= len;
    }

    std::string retrieve_all_as_string()
    {
        return retrieve_as_string(readable_bytes());
    }

    std::string retrieve_as_string(size_t len)
    {
        assert(len <= readable_bytes());
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    void append(const char* data, size_t len)
    {
        ensure_writabel_bytes(len);
        std::copy(data, data + len, begin_write());
        has_written(len);
    }

    void append(const std::string_view& data)
    {
        append(data.data(), data.size());
    }

    void ensure_writabel_bytes(size_t len)
    {
        if (writable_bytes() < len)
        {
            make_space_(len);
        }
        assert(writable_bytes() >= len);
    }

    /// Read data directly into buffer.
    ///
    /// It may implement with readv(2)
    /// @return result of read(2), @c errno is saved
    ssize_t readfd(int fd, int* saved_errno);

    const char* find_crlf() const
    {
        const char* crlf = std::search(peek(), begin_write(), kCRLF, kCRLF + 2);
        return crlf == begin_write() ? nullptr : crlf;
    }

    const char* find_crlf(const char* start) const
    {
        assert(peek() <= start);
        assert(start <= begin_write());
        const char* crlf = std::search(start, begin_write(), kCRLF, kCRLF + 2);
        return crlf == begin_write() ? nullptr : crlf;            
    }
    
private:
    char* begin()
    {
        return buffer_.data();
    }

    const char* begin() const
    {
        return buffer_.data();
    }

    void make_space_(size_t len);

private:
    std::vector<char> buffer_;
    size_t reader_index_;
    size_t writer_index_;
    
    static const char kCRLF[];
};
#include "src/Buffer.h"

#include <algorithm>
#include <sys/uio.h>
#include <errno.h>

const char Buffer::kCRLF[] = "\r\n";

Buffer::Buffer(size_t initial_size)
    : buffer_(kCheapPrepend + initial_size)
    , reader_index_(kCheapPrepend)
    , writer_index_(kCheapPrepend)
{
    assert(readable_bytes() == 0);
    assert(writable_bytes() == initial_size);
    assert(prependable_bytes() == kCheapPrepend);
}

Buffer::Buffer(Buffer&& rhs)
    : buffer_(std::move(rhs.buffer_))
    , reader_index_(rhs.reader_index_)
    , writer_index_(rhs.writer_index_)
{
    rhs.reader_index_ = kCheapPrepend;
    rhs.writer_index_ = kCheapPrepend;
}

Buffer& Buffer::operator=(Buffer&& rhs)
{
    if (&rhs != this)
    {
        buffer_ = std::move(rhs.buffer_);
        reader_index_ = rhs.reader_index_;
        writer_index_ = rhs.writer_index_;
    }
    return *this;
}

void Buffer::swap(Buffer& rhs)
{
    buffer_.swap(rhs.buffer_);
    std::swap(reader_index_, rhs.reader_index_);
    std::swap(writer_index_, rhs.writer_index_);
}

void Buffer::make_space_(size_t len)
{
    if (writable_bytes() + prependable_bytes() < len + kCheapPrepend)
    {
        buffer_.resize(writer_index_ + len);
    }
    else
    {
        assert(kCheapPrepend < reader_index_);
        size_t readable = readable_bytes();
        std::copy(begin() + reader_index_, begin() + writer_index_, begin() + kCheapPrepend);
        reader_index_ = kCheapPrepend;
        writer_index_ = reader_index_ + readable;
        assert(readable == readable_bytes());
    }
}

ssize_t Buffer::readfd(int fd, int* saved_errno)
{
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writable_bytes();
    vec[0].iov_base = begin() + writer_index_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    // when there is enough space in this buffer, don't read into extrabuf.
    // when extrabuf is used, we read 128k-1 bytes at most.
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0)
    {
        *saved_errno = errno;
    }
    else if (static_cast<size_t>(n) <= writable)
    {
        writer_index_ += n;
    }
    else
    {
        writer_index_ = buffer_.size();
        append(extrabuf, n - writable);
    }

    return n;
}
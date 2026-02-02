// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>

//
//------------------------------------------------------------------------------
// Minimal Status for the reactor boundary.
//------------------------------------------------------------------------------
//
struct Status
{
    bool ok = true;
    std::string message{};

    static Status Ok()
    {
        return Status{};
    }

    static Status Error(std::string msg)
    {
        Status s;
        s.ok = false;
        s.message = std::move(msg);
        return s;
    }
};

//
//------------------------------------------------------------------------------
// Minimal SensorContext boundary for the reactor.
//------------------------------------------------------------------------------
//
class SensorContext
{
public:
    virtual ~SensorContext() = default;

    virtual int fd() const noexcept = 0;

    virtual Status on_readable() noexcept = 0;
};

//
//------------------------------------------------------------------------------
// epoll reactor
//------------------------------------------------------------------------------
class EpollReactor
{
public:
    EpollReactor() = default;

    ~EpollReactor()
    {
        if (mEpollFd >= 0)
        {
            ::close(mEpollFd);
            mEpollFd = -1;
        }
    }

    EpollReactor(const EpollReactor&) = delete;
    EpollReactor& operator=(const EpollReactor&) = delete;

    EpollReactor(EpollReactor&& other) noexcept
        : mEpollFd(other.mEpollFd)
    {
        other.mEpollFd = -1;
    }

    EpollReactor& operator=(EpollReactor&& other) noexcept
    {
        if (this == &other) return *this;

        if (mEpollFd >= 0)
        {
            ::close(mEpollFd);
        }

        mEpollFd = other.mEpollFd;
        other.mEpollFd = -1;
        return *this;
    }

    Status open() noexcept
    {
        mEpollFd = ::epoll_create1(0);
        if (mEpollFd < 0)
        {
            return Status::Error(std::string("epoll_create1 failed: ") + std::strerror(errno));
        }

        return Status::Ok();
    }

    Status add(SensorContext& ctx) noexcept
    {
        if (mEpollFd < 0)
        {
            return Status::Error("EpollReactor not opened");
        }

        const int fd = ctx.fd();
        if (fd < 0)
        {
            return Status::Error("SensorContext has invalid fd");
        }

        epoll_event ev{};
        ev.events = EPOLLIN;               // level-triggered read readiness
        ev.data.ptr = &ctx;                // the important part

        const int rc = ::epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fd, &ev);
        if (rc < 0)
        {
            return Status::Error(std::string("epoll_ctl(ADD) failed: ") + std::strerror(errno));
        }

        return Status::Ok();
    }

    Status remove(SensorContext& ctx) noexcept
    {
        if (mEpollFd < 0)
        {
            return Status::Error("EpollReactor not opened");
        }

        const int fd = ctx.fd();
        if (fd < 0)
        {
            return Status::Error("SensorContext has invalid fd");
        }

        // epoll_ctl ignores ev for DEL on Linux, but pass nullptr explicitly.
        const int rc = ::epoll_ctl(mEpollFd, EPOLL_CTL_DEL, fd, nullptr);
        if (rc < 0)
        {
            return Status::Error(std::string("epoll_ctl(DEL) failed: ") + std::strerror(errno));
        }

        return Status::Ok();
    }

    // Run one wait/dispatch step. Good for demo and easy testing.
    Status poll_once(int timeout_ms) noexcept
    {
        if (mEpollFd < 0)
        {
            return Status::Error("EpollReactor not opened");
        }

        epoll_event events[MaxEvents]{};

        const int n = ::epoll_wait(mEpollFd, events, MaxEvents, timeout_ms);
        if (n < 0)
        {
            if (errno == EINTR)
            {
                return Status::Ok();
            }
            return Status::Error(std::string("epoll_wait failed: ") + std::strerror(errno));
        }

        if (n == 0)
        {
            return Status::Ok(); // timeout
        }

        for (int i = 0; i < n; ++i)
        {
            auto* ctx = reinterpret_cast<SensorContext*>(events[i].data.ptr);
            if (!ctx)
            {
                continue;
            }

            // Chapter 10 example: reactor does not interpret flags.
            // It just calls on_readable. The context drains until EAGAIN.
            Status s = ctx->on_readable();
            if (!s.ok)
            {
                // Pedagogical: print and continue.
                // Real system could track counters or signal shutdown.
                std::cerr << "on_readable error: " << s.message << "\n";
            }
        }

        return Status::Ok();
    }

private:
    static constexpr int MaxEvents = 16;

private:
    int mEpollFd = -1;
};

//
//------------------------------------------------------------------------------
// Demo stub: UDP socket context (only to let this compile as a standalone test).
//------------------------------------------------------------------------------
//
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

static Status set_nonblocking(int fd) noexcept
{
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
        return Status::Error(std::string("fcntl(F_GETFL) failed: ") + std::strerror(errno));
    }

    if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        return Status::Error(std::string("fcntl(F_SETFL) failed: ") + std::strerror(errno));
    }

    return Status::Ok();
}

static int make_udp_port(int port) noexcept
{
    const int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        return -1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (::bind(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        ::close(fd);
        return -1;
    }

    return fd;
}

class UdpPrintContext final : public SensorContext
{
public:
    explicit UdpPrintContext(int port)
    {
        mFd = make_udp_port(port);
        std::cout << "mFd is " << mFd << std::endl;
        if (mFd >= 0)
        {
            (void)set_nonblocking(mFd);
        }
    }

    ~UdpPrintContext() override
    {
        if (mFd >= 0)
        {
            ::close(mFd);
            mFd = -1;
        }
    }

    int fd() const noexcept override
    {
        return mFd;
    }

    Status on_readable() noexcept override
    {
        if (mFd < 0)
        {
            return Status::Error("UdpPrintContext has invalid fd");
        }

        // Drain until EAGAIN (important for correctness, even level-triggered).
        for (;;)
        {
            sockaddr_in src{};
            socklen_t srclen = sizeof(src);

            char buf[1024];
            const ssize_t n = ::recvfrom(
                mFd,
                buf,
                sizeof(buf) - 1,
                0,
                reinterpret_cast<sockaddr*>(&src),
                &srclen);

            if (n < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    return Status::Ok();
                }
                return Status::Error(std::string("recvfrom failed: ") + std::strerror(errno));
            }

            buf[n] = '\0';
            std::cout << "UDP rx: '" << buf << "'\n";
        }
    }

private:
    int mFd = -1;
};

int main()
{
    std::cout << "main entry" << std::endl;

    EpollReactor reactor;
    Status s = reactor.open();
    if (!s.ok)
    {
        std::cerr << s.message << "\n";
        return 1;
    }

    std::cout << "Reactor is open" << std::endl;

    UdpPrintContext ctx(3450);
    if (ctx.fd() < 0)
    {
        std::cerr << "Failed to bind UDP port 3450\n";
        return 2;
    }

    std::cout << "Bound to port # 3450" << std::endl;

    s = reactor.add(ctx);
    std::cout << "After reactor.add" << std::endl;
    if (!s.ok)
    {
        return 3;
    }

    std::cout << "Listening on UDP 3450...\n";

    // Demo: poll a few times
    for (int i = 0; i < 1000; ++i)
    {
        (void)reactor.poll_once(10000);
    }

    return 0;
}

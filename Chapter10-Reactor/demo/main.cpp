// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include <atomic>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <thread>
#include <chrono>
#include <array>
#include <cerrno>
#include <cstring>

#include <sys/epoll.h>
#include <unistd.h>

#include "readerwriterqueue/readerwriterqueue.h"

#include "AnyMeasurement.h"
#include "SensorContext.h"
#include "UdpSensor.h"
#include "UartSensor.h"
#include "PortStats.h"

using namespace std::chrono_literals;

static std::atomic<bool> gRunning{true};

static void on_sigint(int)
{
    gRunning.store(false);
}

int main()
{
    std::signal(SIGINT, &on_sigint);

    moodycamel::ReaderWriterQueue<weather::AnyMeasurement, 128> q;

    std::array<weather::SensorContext, 2> sensors{};

    // UDP port 3450, max message size 2048.
    weather::configure_udp_sensor(sensors[0], 3450, q, 2048);

// UART NMEA line framing via socat PTYs.
// Terminal 1:
//   socat -d -d pty,raw,echo=0,link=/tmp/uartA pty,raw,echo=0,link=/tmp/uartB
// Terminal 2 (send a line):
//   echo -ne '$GPGGA,HELLO*00\r\n' > /tmp/uartB
weather::configure_uart_nmea_sensor(sensors[1], "/tmp/uartA", 9600, q, 128);

    const int epfd = ::epoll_create1(0);
    if (epfd < 0)
    {
        std::cerr << "epoll_create1 failed: " << std::strerror(errno) << "\n";
        return 1;
    }

    auto add_epoll = [&](weather::SensorContext& ctx) -> bool
    {
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.ptr = &ctx;

        const int fd = ctx.fd();
        if (fd < 0)
        {
            std::cerr << "sensor fd invalid\n";
            return false;
        }

        if (::epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0)
        {
            std::cerr << "epoll_ctl add failed: " << std::strerror(errno) << "\n";
            return false;
        }

        return true;
    };

    if (!add_epoll(sensors[0]))
    {
        ::close(epfd);
        return 2;
    }

    if (!add_epoll(sensors[1]))
    {
        ::close(epfd);
        return 3;
    }

    std::cout << "Listening on UDP 3450 and UART /tmp/uartA...\n";
    std::cout << "Send: echo \"HELLO\" | nc -u 127.0.0.1 3450\n";
    std::cout << "Ctrl-C to stop.\n";

    std::thread consumer([&]
    {
        std::uint64_t seen = 0;
        auto nextReport = std::chrono::steady_clock::now() + 1s;

        while (gRunning.load())
        {
            weather::AnyMeasurement m = weather::AnyMeasurement::empty();

            while (q.try_dequeue(m))
            {
                ++seen;
                std::cout << "Dequeued measurement #" << seen
                          << " kind=" << static_cast<int>(m.kind())
                          << "\n";
            }

            const auto now = std::chrono::steady_clock::now();
            if (now >= nextReport)
            {
                const weather::PortStats* s = weather::udp_stats(sensors[0]);
                const weather::PortStats* u = weather::uart_stats(sensors[1]);
                if (s)
                {
                    std::cout << "UDP Stats: rx=" << s->frames_received
                              << " drop=" << s->frames_dropped
                              << " verify_fail=" << s->verify_failures
                              << "\n";
                }
                if (u)
                {
                    std::cout << "UART Stats: lines=" << u->frames_received
                              << " drop=" << u->frames_dropped
                              << " verify_fail=" << u->verify_failures
                              << "\n";
                }
                nextReport = now + 1s;
            }

            std::this_thread::sleep_for(10ms);
        }
    });

    epoll_event events[8]{};

    while (gRunning.load())
    {
        const int n = ::epoll_wait(epfd, events, 8, 1000);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            std::cerr << "epoll_wait failed: " << std::strerror(errno) << "\n";
            break;
        }

        for (int i = 0; i < n; ++i)
        {
            auto* ctx = reinterpret_cast<weather::SensorContext*>(events[i].data.ptr);
            if (ctx)
            {
                (void)ctx->on_readable();
            }
        }
    }

    gRunning.store(false);
    consumer.join();

    ::close(epfd);

    std::cout << "Done.\n";
    return 0;
}

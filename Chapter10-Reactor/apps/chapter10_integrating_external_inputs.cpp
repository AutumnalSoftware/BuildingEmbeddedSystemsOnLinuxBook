// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include <atomic>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <array>

#include <sys/epoll.h>
#include <unistd.h>

#include "WeatherSystem.h"
#include "WeatherSystemBuilder.h"

#include "SensorContext.h"
#include "UdpSensor.h"
#include "UartSensor.h"

static std::atomic<bool> gStop{false};

static void on_sigint(int)
{
    gStop.store(true);
}

static bool add_epoll(int epfd, weather::SensorContext& ctx)
{
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.ptr = &ctx;

    if (::epoll_ctl(epfd, EPOLL_CTL_ADD, ctx.fd(), &ev) < 0)
    {
        std::cerr << "epoll_ctl add failed: " << std::strerror(errno) << "\n";
        return false;
    }

    return true;
}

int main()
{
    std::signal(SIGINT, &on_sigint);

    WeatherSystem system;

    WeatherSystemBuilder builder;
    BuildStatus bs = builder.build(system);
    if (!bs)
    {
        std::cerr << bs.message << "\n";
        return 1;
    }

    // Thread 0 (producer) becomes "external inputs reactor" for this chapter.
    system.setThreadEntry(0, [&system](const std::atomic<bool>& stop)
    {
        auto& inQ = system.inQueue();

        std::array<weather::SensorContext, 2> sensors{};

        weather::configure_udp_sensor(sensors[0], 3450, inQ, 2048);
        weather::configure_uart_nmea_sensor(sensors[1], "/tmp/uartA", 9600, inQ, 128);

        const int epfd = ::epoll_create1(0);
        if (epfd < 0)
        {
            std::cerr << "epoll_create1 failed: " << std::strerror(errno) << "\n";
            return;
        }

        if (!add_epoll(epfd, sensors[0]) || !add_epoll(epfd, sensors[1]))
        {
            ::close(epfd);
            return;
        }

        std::cout << "Integrating External Inputs:\n";
        std::cout << "  UDP 3450:  echo \"HELLO\" | nc -u 127.0.0.1 3450\n";
        std::cout << "  UART PTYs:\n";
        std::cout << "    socat -d -d pty,raw,echo=0,link=/tmp/uartA pty,raw,echo=0,link=/tmp/uartB\n";
        std::cout << "    echo -ne '$GPGGA,HELLO*00\\r\\n' > /tmp/uartB\n";
        std::cout << "Ctrl-C to stop.\n";

        epoll_event events[8]{};

        while (!stop.load(std::memory_order_relaxed))
        {
            const int n = ::epoll_wait(epfd, events, 8, 250);
            if (n < 0)
            {
                if (errno == EINTR) continue;
                std::cerr << "epoll_wait failed: " << std::strerror(errno) << "\n";
                break;
            }

            for (int i = 0; i < n; ++i)
            {
                auto* ctx = reinterpret_cast<weather::SensorContext*>(events[i].data.ptr);
                if (ctx) (void)ctx->on_readable();
            }
        }

        ::close(epfd);
    });

    system.start();

    while (!gStop.load())
    {
        ::usleep(50 * 1000);
    }

    system.stop();
    system.join();
    return 0;
}

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

    system.start();

    while (!gStop.load())
    {
        ::usleep(50 * 1000);
    }

    system.stop();
    system.join();
    return 0;
}

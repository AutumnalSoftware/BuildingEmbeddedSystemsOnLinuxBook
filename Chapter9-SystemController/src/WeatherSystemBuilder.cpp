// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include "WeatherSystemBuilder.h"
#include "Chapter9RunLoops.h"

BuildStatus WeatherSystemBuilder::build(WeatherSystem& system) const
{
    system.setThreadEntry(0, [&system](const std::atomic<bool>& stop)
    {
        system.runLoops().producer(stop);
    });

    system.setThreadEntry(1, [&system](const std::atomic<bool>& stop)
    {
        system.runLoops().consumer(stop);
    });

    system.setThreadEntry(2, [&system](const std::atomic<bool>& stop)
    {
        system.runLoops().logger(stop);
    });

    return BuildStatus::success();
}

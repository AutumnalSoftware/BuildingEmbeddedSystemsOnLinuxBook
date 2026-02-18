// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software
#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "ValueGenerator.h"

namespace weather
{

struct UdpEndpoint
{
    std::string host;
    std::uint16_t port = 0;
};

struct Options
{
    std::optional<std::string> uart_device;
    int uart_baud = 115200;

    std::optional<UdpEndpoint> udp;

    double temp_hz = 0.0;
    double pressure_hz = 0.0;
    double humidity_hz = 0.0;
    double position_hz = 0.0;
    double wind_hz = 0.0;

    GeneratorKind gen = GeneratorKind::RandomWalk;
    std::uint32_t seed = 1;

    double duration_sec = 0.0;
    double log_every_sec = 5.0;
};

}

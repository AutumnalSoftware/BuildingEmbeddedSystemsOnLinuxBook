// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#pragma once

#include <cstdint>
#include <string>
#include <ostream>

namespace weather
{

enum class SourceId : std::uint16_t
{
    Unknown = 0,

    // Simulated sources
    SimTemperature = 100,
    SimPosition    = 101,

    // External inputs
    UdpSensor1     = 200,
    UartSensor1    = 201
};

// Conversion
std::string to_string(SourceId id);
bool from_string(const std::string& s, SourceId& out);

// Stream operator
std::ostream& operator<<(std::ostream& os, SourceId id);

} // namespace weather


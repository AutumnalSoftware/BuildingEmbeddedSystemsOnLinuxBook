// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include "SourceId.h"

namespace weather
{

std::string to_string(SourceId id)
{
    switch (id)
    {
    case SourceId::Unknown:        return "Unknown";
    case SourceId::SimTemperature: return "SimTemperature";
    case SourceId::SimPosition:    return "SimPosition";
    case SourceId::UdpSensor1:     return "UdpSensor1";
    case SourceId::UartSensor1:    return "UartSensor1";
    }

    return "Unknown";
}

bool from_string(const std::string& s, SourceId& out)
{
    if (s == "Unknown")        { out = SourceId::Unknown; return true; }
    if (s == "SimTemperature") { out = SourceId::SimTemperature; return true; }
    if (s == "SimPosition")    { out = SourceId::SimPosition; return true; }
    if (s == "UdpSensor1")     { out = SourceId::UdpSensor1; return true; }
    if (s == "UartSensor1")    { out = SourceId::UartSensor1; return true; }

    return false;
}

std::ostream& operator<<(std::ostream& os, SourceId id)
{
    return os << to_string(id);
}

} // namespace weather

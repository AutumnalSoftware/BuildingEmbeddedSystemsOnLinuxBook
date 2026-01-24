// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#pragma once

#include "MeasurementTypes.h"

#include <ostream>
#include <string_view>

namespace weather
{

inline std::ostream& operator<<(std::ostream& os, MeasurementKind k)
{
    os << to_string(k);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, SourceId s)
{
    switch (s)
    {
    case SourceId::Unknown: os << "Unknown"; break;
    default:                os << "SourceId(unknown)"; break;
    }
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const MeasurementHeaderV1& h)
{
    os << "MeasurementHeaderV1{"
       << "rxTime=" << h.rxTime
       << ", eventTime=" << h.eventTime
       << ", kind=" << h.kind
       << ", source=" << h.source
       << ", flags=" << h.flags
       << "}";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Empty&)
{
    os << "Empty{}";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Temperature& v)
{
    os << "Temperature{value=" << v.value << "}";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const BarometricPressure& v)
{
    os << "BarometricPressure{value=" << v.value << "}";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Humidity& v)
{
    os << "Humidity{value=" << v.value << "}";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const WindSpeed& v)
{
    os << "WindSpeed{value=" << v.value << "}";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const WindDirection& v)
{
    os << "WindDirection{value=" << v.value << "}";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Precipitation& v)
{
    os << "Precipitation{value=" << v.value << "}";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Position& p)
{
    os << "Position{lat=" << p.lat
       << ", lon=" << p.lon
       << ", alt=" << p.alt
       << "}";
    return os;
}

// Closed-world mapping. If you add a kind, you add it here.
inline std::string_view to_string(MeasurementKind k) noexcept
{
    switch (k)
    {
    case MeasurementKind::Empty:              return "Empty";
    case MeasurementKind::Temperature:        return "Temperature";
    case MeasurementKind::BarometricPressure: return "BarometricPressure";
    case MeasurementKind::Humidity:           return "Humidity";
    case MeasurementKind::WindSpeed:          return "WindSpeed";
    case MeasurementKind::WindDirection:      return "WindDirection";
    case MeasurementKind::Precipitation:      return "Precipitation";
    case MeasurementKind::Position:           return "Position";
    default:                                  return "MeasurementKind(unknown)";
    }
}

inline bool from_string(std::string_view s, MeasurementKind& out) noexcept
{
    if (s == "Empty")              { out = MeasurementKind::Empty; return true; }
    if (s == "Temperature")        { out = MeasurementKind::Temperature; return true; }
    if (s == "BarometricPressure") { out = MeasurementKind::BarometricPressure; return true; }
    if (s == "Humidity")           { out = MeasurementKind::Humidity; return true; }
    if (s == "WindSpeed")          { out = MeasurementKind::WindSpeed; return true; }
    if (s == "WindDirection")      { out = MeasurementKind::WindDirection; return true; }
    if (s == "Precipitation")      { out = MeasurementKind::Precipitation; return true; }
    if (s == "Position")           { out = MeasurementKind::Position; return true; }

    return false;
}

} // namespace weather

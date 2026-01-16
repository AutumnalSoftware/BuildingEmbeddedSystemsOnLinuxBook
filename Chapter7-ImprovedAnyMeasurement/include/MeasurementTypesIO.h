// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#pragma once

#include <ostream>

#include "MeasurementTypes.h"

namespace weather
{

inline std::ostream& operator<<(std::ostream& os, MeasurementKind k)
{
    switch (k)
    {
    case MeasurementKind::Temperature:        os << "Temperature"; break;
    case MeasurementKind::BarometricPressure: os << "BarometricPressure"; break;
    case MeasurementKind::Humidity:           os << "Humidity"; break;
    case MeasurementKind::WindSpeed:          os << "WindSpeed"; break;
    case MeasurementKind::WindDirection:      os << "WindDirection"; break;
    case MeasurementKind::Precipitation:      os << "Precipitation"; break;
    case MeasurementKind::Position:           os << "Position"; break;
    default:                                  os << "MeasurementKind(unknown)"; break;
    }
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

} // namespace weather

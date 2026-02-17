// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include "MeasurementTypeStrings.h"

namespace weather
{

std::string_view to_string(MeasurementKind k) noexcept
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

bool from_string(std::string_view s, MeasurementKind& out) noexcept
{
    // Exact-match only. (No allocations, no locale surprises.)
    if (s == "Empty")              { out = MeasurementKind::Empty;              return true; }
    if (s == "Temperature")        { out = MeasurementKind::Temperature;        return true; }
    if (s == "BarometricPressure") { out = MeasurementKind::BarometricPressure; return true; }
    if (s == "Humidity")           { out = MeasurementKind::Humidity;           return true; }
    if (s == "WindSpeed")          { out = MeasurementKind::WindSpeed;          return true; }
    if (s == "WindDirection")      { out = MeasurementKind::WindDirection;      return true; }
    if (s == "Precipitation")      { out = MeasurementKind::Precipitation;      return true; }
    if (s == "Position")           { out = MeasurementKind::Position;           return true; }

    return false;
}

} // namespace weather

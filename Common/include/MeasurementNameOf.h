// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software
#pragma once

#include <string_view>

#include "MeasurementTypes.h"

namespace weather
{

template <typename T>
struct MeasurementNameOf; // intentionally undefined for non-measurement types

template <>
struct MeasurementNameOf<Empty>
{
    static constexpr std::string_view value = "Empty";
};

template <>
struct MeasurementNameOf<Temperature>
{
    static constexpr std::string_view value = "Temperature";
};

template <>
struct MeasurementNameOf<BarometricPressure>
{
    static constexpr std::string_view value = "BarometricPressure";
};

template <>
struct MeasurementNameOf<Humidity>
{
    static constexpr std::string_view value = "Humidity";
};

template <>
struct MeasurementNameOf<WindSpeed>
{
    static constexpr std::string_view value = "WindSpeed";
};

template <>
struct MeasurementNameOf<WindDirection>
{
    static constexpr std::string_view value = "WindDirection";
};

template <>
struct MeasurementNameOf<Precipitation>
{
    static constexpr std::string_view value = "Precipitation";
};

template <>
struct MeasurementNameOf<Position>
{
    static constexpr std::string_view value = "Position";
};

} // namespace weather

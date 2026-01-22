// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#pragma once

#include <cstdint>

namespace weather {

enum class MeasurementKind : std::uint16_t {
    Empty = 0,
    Temperature,
    BarometricPressure,
    Humidity,
    WindSpeed,
    WindDirection,
    Precipitation,
    Position
};

enum class SourceId : std::uint16_t {
    Unknown
};

struct MeasurementHeaderV1 {
    std::uint64_t rxTime = 0;
    std::uint64_t eventTime = 0;
    MeasurementKind kind = MeasurementKind::Empty;
    SourceId source = SourceId::Unknown;
    std::uint32_t flags = 0;
};

// Empty payload used for scratch / sentinel measurements
struct Empty {};

// Concrete measurement payloads
struct Temperature        { double value = 0.0; };
struct BarometricPressure { double value = 0.0; };
struct Humidity           { double value = 0.0; };
struct WindSpeed          { double value = 0.0; };
struct WindDirection      { double value = 0.0; };
struct Precipitation      { double value = 0.0; };
struct Position           { double lat = 0.0; double lon = 0.0; double alt = 0.0; };

template <typename T> struct MeasurementKindOf;

template <> struct MeasurementKindOf<Empty>              { static constexpr MeasurementKind value = MeasurementKind::Empty; };
template <> struct MeasurementKindOf<Temperature>        { static constexpr MeasurementKind value = MeasurementKind::Temperature; };
template <> struct MeasurementKindOf<BarometricPressure> { static constexpr MeasurementKind value = MeasurementKind::BarometricPressure; };
template <> struct MeasurementKindOf<Humidity>           { static constexpr MeasurementKind value = MeasurementKind::Humidity; };
template <> struct MeasurementKindOf<WindSpeed>          { static constexpr MeasurementKind value = MeasurementKind::WindSpeed; };
template <> struct MeasurementKindOf<WindDirection>      { static constexpr MeasurementKind value = MeasurementKind::WindDirection; };
template <> struct MeasurementKindOf<Precipitation>      { static constexpr MeasurementKind value = MeasurementKind::Precipitation; };
template <> struct MeasurementKindOf<Position>           { static constexpr MeasurementKind value = MeasurementKind::Position; };

} // namespace weather

#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>

namespace weather
{
    enum class MeasurementKind : std::uint16_t
    {
        Temperature = 0,
        BarometricPressure,
        Humidity,
        WindSpeed,
        WindDirection,
        Precipitation,
        Position,
    };

    std::string to_string(MeasurementKind kind);
    bool from_string(const std::string& s, MeasurementKind& out);
    std::ostream& operator<<(std::ostream& os, MeasurementKind kind);

    enum class SourceId : std::uint16_t
    {
        WeatherSensors = 0,
        Gps
    };

    std::string to_string(SourceId src);
    bool from_string(const std::string& s, SourceId& out);
    std::ostream& operator<<(std::ostream& os, SourceId src);

    struct MeasurementHeaderV1
    {
        std::uint64_t rxTime = 0;
        std::uint64_t eventTime = 0;
        MeasurementKind kind = MeasurementKind::Temperature;
        SourceId source = SourceId::WeatherSensors;
        std::uint32_t flags = 0;
    };

    std::ostream& operator<<(std::ostream& os, const MeasurementHeaderV1& h);

    struct Temperature { float celsius = 0.0f; };
    struct BarometricPressure { float hectopascals = 0.0f; };
    struct Humidity { float relative_percent = 0.0f; };
    struct WindSpeed { float meters_per_sec = 0.0f; };
    struct WindDirection { float degrees = 0.0f; };
    struct Precipitation { float rate_mm_per_hr = 0.0f; float accumulation_mm = 0.0f; };

    struct Position
    {
        double latitude_deg = 0.0;
        double longitude_deg = 0.0;
        float altitude_m = 0.0f;
    };

    template <typename T>
    struct MeasurementKindOf;

    template <> struct MeasurementKindOf<Temperature> { static constexpr MeasurementKind value = MeasurementKind::Temperature; };
    template <> struct MeasurementKindOf<BarometricPressure> { static constexpr MeasurementKind value = MeasurementKind::BarometricPressure; };
    template <> struct MeasurementKindOf<Humidity> { static constexpr MeasurementKind value = MeasurementKind::Humidity; };
    template <> struct MeasurementKindOf<WindSpeed> { static constexpr MeasurementKind value = MeasurementKind::WindSpeed; };
    template <> struct MeasurementKindOf<WindDirection> { static constexpr MeasurementKind value = MeasurementKind::WindDirection; };
    template <> struct MeasurementKindOf<Precipitation> { static constexpr MeasurementKind value = MeasurementKind::Precipitation; };
    template <> struct MeasurementKindOf<Position> { static constexpr MeasurementKind value = MeasurementKind::Position; };

} // namespace weather

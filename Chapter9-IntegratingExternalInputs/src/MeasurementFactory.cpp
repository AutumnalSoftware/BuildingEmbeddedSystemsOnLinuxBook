// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include "MeasurementFactory.h"

namespace chapter9
{
    std::uint64_t to_ns(Clock::time_point t) noexcept
    {
        return static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count());
    }

    weather::AnyMeasurement make_temperature(double seed, Clock::time_point eventTime, Clock::time_point rxTime)
    {
        weather::MeasurementHeaderV1 h;
        h.eventTime = to_ns(eventTime);
        h.rxTime = to_ns(rxTime);
        h.source = weather::SourceId::Unknown;
        h.flags = 0;
        // kind is set by AnyMeasurement via MeasurementKindOf<T>

        weather::Temperature t;
        t.value = 20.0 + seed;

        return weather::AnyMeasurement(h, t);
    }

    weather::AnyMeasurement make_position(double seed, Clock::time_point eventTime, Clock::time_point rxTime)
    {
        weather::MeasurementHeaderV1 h;
        h.eventTime = to_ns(eventTime);
        h.rxTime = to_ns(rxTime);
        h.source = weather::SourceId::Unknown;
        h.flags = 0;

        weather::Position p;
        p.lat = 43.0 + seed * 0.0001;
        p.lon = -77.0 + seed * 0.0001;
        p.alt = 150.0;

        return weather::AnyMeasurement(h, p);
    }
}

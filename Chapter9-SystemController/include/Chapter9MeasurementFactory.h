// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#pragma once

#include <chrono>
#include <cstdint>

#include "AnyMeasurement.h"
#include "MeasurementTypes.h"

namespace chapter9
{
    using Clock = std::chrono::steady_clock;

    // Convert steady_clock time_point to a monotonic nanosecond tick count.
    std::uint64_t to_ns(Clock::time_point t) noexcept;

    weather::AnyMeasurement make_temperature(double seed, Clock::time_point eventTime, Clock::time_point rxTime);
    weather::AnyMeasurement make_position(double seed, Clock::time_point eventTime, Clock::time_point rxTime);
}

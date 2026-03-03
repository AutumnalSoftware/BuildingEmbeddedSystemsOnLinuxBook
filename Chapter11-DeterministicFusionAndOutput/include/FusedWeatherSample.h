// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#pragma once

#include <cstdint>
#include <ostream>

#include "MeasurementTypes.h"

namespace weather
{

struct FusedWeatherSample
{
    Temperature temperature{};
    Position    position{};

    std::uint64_t tempRxTimeNs{0};
    std::uint64_t posRxTimeNs{0};
    std::uint64_t dtNs{0};
};

inline std::ostream& operator<<(std::ostream& os, const FusedWeatherSample& s)
{
    os << "[FUSED] "
       << "pos=(" << s.position.lat << ", "
       << s.position.lon << ", "
       << s.position.alt << ") "
       << "temp=" << s.temperature.value << "C "
       << "dt=" << (static_cast<double>(s.dtNs) / 1'000'000.0) << "ms";

    return os;
}

} // namespace weather

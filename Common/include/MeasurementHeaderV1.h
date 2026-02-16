// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#pragma once

#include <cstdint>

#include "SourceId.h"

namespace weather {

struct MeasurementHeaderV1 {
    std::uint64_t rxTime = 0;
    std::uint64_t eventTime = 0;
    MeasurementKind kind = MeasurementKind::Empty;
    SourceId source = SourceId::Unknown;
    std::uint32_t flags = 0;
};

} // namespace weather

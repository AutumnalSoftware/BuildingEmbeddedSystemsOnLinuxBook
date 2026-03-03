// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#pragma once

#include <cstddef>
#include <cstdint>
#include "AnyMeasurement.h"
#include "PortStats.h"
#include "SystemStats.h"
#include "readerwriterqueue/readerwriterqueue.h"

namespace weather
{
class SensorContext;

void configure_udp_sensor(SensorContext& ctx,
                          std::uint16_t port,
                          moodycamel::ReaderWriterQueue<AnyMeasurement, 128>& outQ,
                          std::size_t maxMessageSize,
                          SystemStats& sysStats) noexcept;

const PortStats* udp_stats(const SensorContext& ctx) noexcept;
}

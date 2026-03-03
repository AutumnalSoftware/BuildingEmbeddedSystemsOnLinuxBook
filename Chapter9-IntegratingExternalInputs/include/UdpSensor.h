#pragma once

#include <cstddef>
#include <cstdint>
#include "AnyMeasurement.h"
#include "PortStats.h"
#include "readerwriterqueue/readerwriterqueue.h"

namespace weather
{
class SensorContext;

void configure_udp_sensor(SensorContext& ctx,
                          std::uint16_t port,
                          moodycamel::ReaderWriterQueue<AnyMeasurement, 128>& outQ,
                          std::size_t maxMessageSize) noexcept;

const PortStats* udp_stats(const SensorContext& ctx) noexcept;
}

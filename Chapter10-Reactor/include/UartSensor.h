#pragma once

#include <cstddef>
#include <cstdint>

#include "AnyMeasurement.h"
#include "PortStats.h"
#include "readerwriterqueue/readerwriterqueue.h"

namespace weather
{
class SensorContext;

void configure_uart_nmea_sensor(SensorContext& ctx,
                                const char* devicePath,
                                int baud,
                                moodycamel::ReaderWriterQueue<AnyMeasurement, 128>& outQ,
                                std::size_t maxLineBytes = 128) noexcept;

const PortStats* uart_stats(const SensorContext& ctx) noexcept;
}

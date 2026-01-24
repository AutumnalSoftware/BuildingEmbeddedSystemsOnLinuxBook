// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#pragma once

#include <atomic>

#include "AnyMeasurement.h"
#include "LogEvent.h"
#include "readerwriterqueue.h"

struct Chapter9Stats
{
    // Producer-side
    std::atomic<std::uint64_t> enqTempOk{0};
    std::atomic<std::uint64_t> enqPosOk{0};
    std::atomic<std::uint64_t> enqDrops{0};

    // Measurement queue depth tracking (manual, works with any queue)
    std::atomic<std::int32_t>  inQDepth{0};
    std::atomic<std::uint32_t> inQHighWater{0};

    // Consumer-side
    std::atomic<std::uint64_t> deqTemp{0};
    std::atomic<std::uint64_t> deqPos{0};

    // End-to-end latency (max observed)
    std::atomic<std::uint64_t> latencyMaxNs{0};

    // Logger queue (optional, same pattern)
    std::atomic<std::int32_t>  logQDepth{0};
    std::atomic<std::uint32_t> logQHighWater{0};
};


class Chapter9RunLoops
{
public:
    Chapter9RunLoops(moodycamel::ReaderWriterQueue<weather::AnyMeasurement>& inQ,
                     moodycamel::ReaderWriterQueue<LogEvent>& logQ);

    void producer(const std::atomic<bool>& stop);
    void consumer(const std::atomic<bool>& stop);
    void logger(const std::atomic<bool>& stop);

private:
    moodycamel::ReaderWriterQueue<weather::AnyMeasurement>& m_inQ;
    moodycamel::ReaderWriterQueue<LogEvent>& m_logQ;

    Chapter9Stats m_stats;
    void printStatsLine(std::uint64_t seconds);
};

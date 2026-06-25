// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#pragma once

#include <atomic>

#include "AnyMeasurement.h"
#include "LogEvent.h"
#include "readerwriterqueue.h"

struct Chapter8Stats
{
    std::atomic<std::uint64_t> enqTempOk{0};
    std::atomic<std::uint64_t> enqPosOk{0};

    std::atomic<std::uint64_t> deqTemp{0};
    std::atomic<std::uint64_t> deqPos{0};

    std::atomic<std::uint64_t> enqDrops{0};

    std::atomic<std::uint64_t> inQDepth{0};
    std::atomic<std::uint64_t> inQDepthHi{0};

    std::atomic<std::uint64_t> latencyMaxNs{0};
};



class Chapter8RunLoops
{
public:
    Chapter8RunLoops(moodycamel::ReaderWriterQueue<weather::AnyMeasurement>& inQ,
                     moodycamel::ReaderWriterQueue<LogEvent>& logQ);

    void producer(const std::atomic<bool>& stop);
    void consumer(const std::atomic<bool>& stop);
    void logger(const std::atomic<bool>& stop);

private:
    moodycamel::ReaderWriterQueue<weather::AnyMeasurement>& m_inQ;
    moodycamel::ReaderWriterQueue<LogEvent>& m_logQ;

    Chapter8Stats m_stats;
};

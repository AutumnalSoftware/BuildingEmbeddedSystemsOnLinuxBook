// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software


#include <iomanip>
#include <iostream>
#include <chrono>
#include <thread>

#include "RunLoops.h"
#include "MeasurementTypes.h"

//
// Helpers
//
static void updateHighWater(std::atomic<std::uint64_t>& high,
                            std::uint64_t value)
{
    std::uint64_t cur = high.load(std::memory_order_relaxed);
    while (value > cur &&
           !high.compare_exchange_weak(cur, value,
                                       std::memory_order_relaxed,
                                       std::memory_order_relaxed))
    {
        // cur updated
    }
}

static void updateMax(std::atomic<std::uint64_t>& maxVal,
                      std::uint64_t value)
{
    std::uint64_t cur = maxVal.load(std::memory_order_relaxed);
    while (value > cur &&
           !maxVal.compare_exchange_weak(cur, value,
                                         std::memory_order_relaxed,
                                         std::memory_order_relaxed))
    {
        // cur updated
    }
}

RunLoops::RunLoops(
    moodycamel::ReaderWriterQueue<weather::AnyMeasurement, 128>& inQ,
    moodycamel::ReaderWriterQueue<LogEvent>& logQ)
    : m_inQ(inQ)
    , m_logQ(logQ)
{
}

void RunLoops::producer(const std::atomic<bool>& stop)
{
    using namespace std::chrono;

    auto nextTemp = steady_clock::now();
    auto nextPos  = steady_clock::now();

    while (!stop.load(std::memory_order_relaxed))
    {
        const auto now = steady_clock::now();

        if (now >= nextTemp)
        {
            weather::MeasurementHeaderV1 h{};
            h.rxTime = duration_cast<nanoseconds>(now.time_since_epoch()).count();
            h.eventTime = h.rxTime;

            weather::Temperature t{25.0};

            if (m_inQ.try_enqueue(weather::AnyMeasurement(h, t)))
            {
                m_stats.enqTempOk.fetch_add(1, std::memory_order_relaxed);

                const auto depth =
                    m_stats.inQDepth.fetch_add(1, std::memory_order_relaxed) + 1;

                updateHighWater(m_stats.inQDepthHi, depth);
            }
            else
            {
                m_stats.enqDrops.fetch_add(1, std::memory_order_relaxed);
            }

            nextTemp += milliseconds(1);
        }

        if (now >= nextPos)
        {
            weather::MeasurementHeaderV1 h{};
            h.rxTime = duration_cast<nanoseconds>(now.time_since_epoch()).count();
            h.eventTime = h.rxTime;

            weather::Position p{42.0, -77.0, 150.0};

            if (m_inQ.try_enqueue(weather::AnyMeasurement(h, p)))
            {
                m_stats.enqPosOk.fetch_add(1, std::memory_order_relaxed);

                const auto depth =
                    m_stats.inQDepth.fetch_add(1, std::memory_order_relaxed) + 1;

                updateHighWater(m_stats.inQDepthHi, depth);
            }
            else
            {
                m_stats.enqDrops.fetch_add(1, std::memory_order_relaxed);
            }

            nextPos += seconds(1);
        }

        std::this_thread::sleep_for(milliseconds(1));
    }
}

void RunLoops::consumer(const std::atomic<bool>& stop)
{
    using namespace std::chrono;

    weather::AnyMeasurement msg = weather::AnyMeasurement::empty();

    while (!stop.load(std::memory_order_relaxed))
    {
        if (m_inQ.try_dequeue(msg))
        {
            m_stats.inQDepth.fetch_sub(1, std::memory_order_relaxed);

            switch (msg.kind())
            {
            case weather::MeasurementKind::Temperature:
                m_stats.deqTemp.fetch_add(1, std::memory_order_relaxed);
                break;

            case weather::MeasurementKind::Position:
                m_stats.deqPos.fetch_add(1, std::memory_order_relaxed);
                break;

            default:
                break;
            }

            const auto now = steady_clock::now();
            const std::uint64_t nowNs =
                static_cast<std::uint64_t>(duration_cast<nanoseconds>(now.time_since_epoch()).count());

            const std::uint64_t rxNs =
                static_cast<std::uint64_t>(msg.header().rxTime);

            if (nowNs >= rxNs)
            {
                updateMax(m_stats.latencyMaxNs, nowNs - rxNs);
            }

            // No per-message logging in Chapter 9.
        }
        else
        {
            std::this_thread::sleep_for(milliseconds(1));
        }
    }
}


void RunLoops::logger(const std::atomic<bool>& stop)
{
    using namespace std::chrono;

    auto start      = steady_clock::now();
    auto lastReport = start;

    std::uint64_t lastEnqTempOk = 0;
    std::uint64_t lastEnqPosOk  = 0;
    std::uint64_t lastDeqTemp   = 0;
    std::uint64_t lastDeqPos    = 0;
    std::uint64_t lastDrops     = 0;

    while (!stop.load(std::memory_order_relaxed))
    {
        const auto now = steady_clock::now();

        if (now - lastReport >= seconds(1))
        {
            const auto tSec =
                duration_cast<seconds>(now - start).count();

            const std::uint64_t enqTempOk =
                m_stats.enqTempOk.load(std::memory_order_relaxed);
            const std::uint64_t enqPosOk =
                m_stats.enqPosOk.load(std::memory_order_relaxed);

            const std::uint64_t deqTemp =
                m_stats.deqTemp.load(std::memory_order_relaxed);
            const std::uint64_t deqPos =
                m_stats.deqPos.load(std::memory_order_relaxed);

            const std::uint64_t drops =
                m_stats.enqDrops.load(std::memory_order_relaxed);

            const std::uint64_t enqTempPerSec = enqTempOk - lastEnqTempOk;
            const std::uint64_t enqPosPerSec  = enqPosOk  - lastEnqPosOk;
            const std::uint64_t deqTempPerSec = deqTemp   - lastDeqTemp;
            const std::uint64_t deqPosPerSec  = deqPos    - lastDeqPos;
            const std::uint64_t dropsPerSec   = drops     - lastDrops;

            lastEnqTempOk = enqTempOk;
            lastEnqPosOk  = enqPosOk;
            lastDeqTemp   = deqTemp;
            lastDeqPos    = deqPos;
            lastDrops     = drops;

            const std::uint64_t depth =
                m_stats.inQDepth.load(std::memory_order_relaxed);

            const std::uint64_t hi =
                m_stats.inQDepthHi.load(std::memory_order_relaxed);

            const std::uint64_t latMaxNs =
                m_stats.latencyMaxNs.exchange(0, std::memory_order_relaxed);

            const double latMaxMs =
                static_cast<double>(latMaxNs) / 1'000'000.0;

            std::cout
                << "[STAT t=" << tSec << "s] "
                << "enq(temp=" << enqTempPerSec << "/s pos=" << enqPosPerSec << "/s) "
                << "deq(temp=" << deqTempPerSec << "/s pos=" << deqPosPerSec << "/s) "
                << "drops=" << drops << " (+" << dropsPerSec << "/s) "
                << "q(depth=" << depth << " hi=" << hi << ") "
                << "latMax=" << std::fixed << std::setprecision(2) << latMaxMs << "ms\n";

            lastReport = now;
        }
        else
        {
            std::this_thread::sleep_for(milliseconds(10));
        }
    }
}

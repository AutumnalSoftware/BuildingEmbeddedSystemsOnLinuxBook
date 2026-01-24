// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software


#include <iomanip>
#include <iostream>
#include <chrono>
#include <thread>

#include "Chapter9RunLoops.h"
#include "MeasurementTypes.h"

//
// Helpers
//
static void updateHighWater(std::atomic<std::uint32_t>& high,
                            std::uint32_t value)
{
    std::uint32_t cur = high.load(std::memory_order_relaxed);
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

Chapter9RunLoops::Chapter9RunLoops(
    moodycamel::ReaderWriterQueue<weather::AnyMeasurement>& inQ,
    moodycamel::ReaderWriterQueue<LogEvent>& logQ)
    : m_inQ(inQ)
    , m_logQ(logQ)
{
}

void Chapter9RunLoops::producer(const std::atomic<bool>& stop)
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

                const auto depth = m_stats.inQDepth.fetch_add(1, std::memory_order_relaxed) + 1;
                updateHighWater(m_stats.inQHighWater, static_cast<std::uint32_t>(depth));
            }
            else
            {
                m_stats.enqDrops.fetch_add(1, std::memory_order_relaxed);
            }

            //nextTemp += milliseconds(100);
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

                const auto depth = m_stats.inQDepth.fetch_add(1, std::memory_order_relaxed) + 1;
                updateHighWater(m_stats.inQHighWater, static_cast<std::uint32_t>(depth));
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

void Chapter9RunLoops::consumer(const std::atomic<bool>& stop)
{
    using namespace std::chrono;

    weather::AnyMeasurement msg = weather::AnyMeasurement::empty();

    while (!stop.load(std::memory_order_relaxed))
    {
        if (m_inQ.try_dequeue(msg))
        {
            // Depth bookkeeping (must match +1 on successful enqueue).
            m_stats.inQDepth.fetch_sub(1, std::memory_order_relaxed);

            // Count kinds.
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

            // End-to-end latency max (steady_clock ns on both ends).
            const auto now = steady_clock::now();
            const std::uint64_t nowNs =
                static_cast<std::uint64_t>(duration_cast<nanoseconds>(now.time_since_epoch()).count());

            const std::uint64_t rxNs =
                static_cast<std::uint64_t>(msg.header().rxTime);

            if (nowNs >= rxNs)
            {
                updateMax(m_stats.latencyMaxNs, nowNs - rxNs);
            }

            m_logQ.try_enqueue(LogEvent{"Consumed measurement"});
        }
        else
        {
            std::this_thread::sleep_for(milliseconds(1));
        }
    }
}

void Chapter9RunLoops::logger(const std::atomic<bool>& stop)
{
    using Clock = std::chrono::steady_clock;

    LogEvent e;

    auto nextSummary = Clock::now() + std::chrono::seconds(1);
    std::uint64_t seconds = 0;

    while (!stop.load(std::memory_order_relaxed))
    {
        // Drain a few log events quickly (keeps latency down without busy spin).
        bool didWork = false;
        for (int i = 0; i < 64; ++i)
        {
            if (!m_logQ.try_dequeue(e))
            {
                break;
            }

            didWork = true;
            //
            // This removes "SPAM" from the output so we can clearly see the statistics output
            //
            //std::cout << "[LOG] " << e.message << std::endl;
        }

        const auto now = Clock::now();
        if (now >= nextSummary)
        {
            ++seconds;
            nextSummary += std::chrono::seconds(1);

            // Pull stats and print one summary line.
            // (I’ll show you exactly what to store in a second.)
            printStatsLine(seconds);
            didWork = true;
        }

        if (!didWork)
        {
            // Sleep a little, but don’t oversleep the next summary.
            // 10ms is fine; you can also choose 1ms if you want tighter output.
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    // Optional: drain remaining log events at shutdown.
    while (m_logQ.try_dequeue(e))
    {
        //std::cout << "[LOG] " << e.message << std::endl;
    }
}

void Chapter9RunLoops::printStatsLine(std::uint64_t seconds)
{
    // Totals (producer-side success)
    const std::uint64_t enqTempOk = m_stats.enqTempOk.load(std::memory_order_relaxed);
    const std::uint64_t enqPosOk  = m_stats.enqPosOk.load(std::memory_order_relaxed);

    // Totals (consumer-side)
    const std::uint64_t deqTemp = m_stats.deqTemp.load(std::memory_order_relaxed);
    const std::uint64_t deqPos  = m_stats.deqPos.load(std::memory_order_relaxed);

    // Cumulative drops
    const std::uint64_t drops = m_stats.enqDrops.load(std::memory_order_relaxed);

    // Queue depth + high-water
    const std::int32_t  inQDepth = m_stats.inQDepth.load(std::memory_order_relaxed);
    const std::uint32_t inQHi    = m_stats.inQHighWater.load(std::memory_order_relaxed);

    const std::int32_t  logQDepth = m_stats.logQDepth.load(std::memory_order_relaxed);
    const std::uint32_t logQHi    = m_stats.logQHighWater.load(std::memory_order_relaxed);

    // Max end-to-end latency (ns -> ms)
    const std::uint64_t latMaxNs = m_stats.latencyMaxNs.load(std::memory_order_relaxed);
    const double latMaxMs = static_cast<double>(latMaxNs) / 1'000'000.0;

    // Per-second deltas: keep last totals in function-static locals.
    // This is simple and good enough for a demo.
    static std::uint64_t lastEnqTempOk = 0;
    static std::uint64_t lastEnqPosOk  = 0;
    static std::uint64_t lastDeqTemp   = 0;
    static std::uint64_t lastDeqPos    = 0;
    static std::uint64_t lastDrops     = 0;

    const std::uint64_t enqTempRate = enqTempOk - lastEnqTempOk;
    const std::uint64_t enqPosRate  = enqPosOk  - lastEnqPosOk;
    const std::uint64_t deqTempRate = deqTemp   - lastDeqTemp;
    const std::uint64_t deqPosRate  = deqPos    - lastDeqPos;
    const std::uint64_t dropsRate   = drops     - lastDrops;

    lastEnqTempOk = enqTempOk;
    lastEnqPosOk  = enqPosOk;
    lastDeqTemp   = deqTemp;
    lastDeqPos    = deqPos;
    lastDrops     = drops;

    std::cout
        << "[STAT t=" << seconds << "s]"
        << " enq(temp=" << enqTempRate << "/s pos=" << enqPosRate << "/s)"
        << " deq(temp=" << deqTempRate << "/s pos=" << deqPosRate << "/s)"
        << " drops=" << drops << " (+" << dropsRate << "/s)"
        << " q(depth=" << inQDepth << " hi=" << inQHi << ")"
        << " latMax=" << std::fixed << std::setprecision(2) << latMaxMs << "ms"
        << std::endl;

    m_stats.latencyMaxNs.store(0, std::memory_order_relaxed);
    m_stats.inQHighWater.store(static_cast<std::uint32_t>(
                                   m_stats.inQDepth.load(std::memory_order_relaxed)),
                               std::memory_order_relaxed);
}

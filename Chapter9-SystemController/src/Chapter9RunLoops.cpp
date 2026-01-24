// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include "Chapter9RunLoops.h"

#include <chrono>
#include <thread>

#include "MeasurementTypes.h"

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
            m_inQ.try_enqueue(weather::AnyMeasurement(h, t));
            nextTemp += milliseconds(100);
        }

        if (now >= nextPos)
        {
            weather::MeasurementHeaderV1 h{};
            h.rxTime = duration_cast<nanoseconds>(now.time_since_epoch()).count();
            h.eventTime = h.rxTime;

            weather::Position p{42.0, -77.0, 150.0};
            m_inQ.try_enqueue(weather::AnyMeasurement(h, p));
            nextPos += seconds(1);
        }

        std::this_thread::sleep_for(milliseconds(1));
    }
}

void Chapter9RunLoops::consumer(const std::atomic<bool>& stop)
{
    // Scratch object: AnyMeasurement has no default constructor, so we seed one instance for try_dequeue() to write into.
    weather::MeasurementHeaderV1 scratchHeader{};
    scratchHeader.rxTime = 0;
    scratchHeader.eventTime = 0;
    scratchHeader.source = weather::SourceId::Unknown;
    scratchHeader.flags = 0;

    weather::AnyMeasurement msg(scratchHeader, weather::Position{});

    while (!stop.load(std::memory_order_relaxed))
    {
        if (m_inQ.try_dequeue(msg))
        {
            m_logQ.try_enqueue(LogEvent{"Consumed measurement"});
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void Chapter9RunLoops::logger(const std::atomic<bool>& stop)
{
    LogEvent e;
    while (!stop.load(std::memory_order_relaxed))
    {
        if (m_logQ.try_dequeue(e))
        {
            // placeholder for logging
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

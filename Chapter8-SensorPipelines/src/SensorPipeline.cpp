// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software


#include "SensorPipeline.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

#include "MeasurementTypes.h"   // weather::Position, MeasurementHeaderV1, SourceId
#include "PositionProducer.h"
#include "PositionConsumer.h"

namespace
{
using Clock = std::chrono::steady_clock;

std::uint64_t nowNs() noexcept
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            Clock::now().time_since_epoch()).count());
}
} // namespace

SensorPipeline::SensorPipeline(std::size_t capacity)
    : m_queue(capacity)
{
}

SensorPipeline::~SensorPipeline()
{
    stop();
    join();
}

void SensorPipeline::start()
{
    if (m_running.exchange(true))
    {
        return;
    }

    m_producerThread = std::thread(&SensorPipeline::producerLoop, this);
    m_consumerThread = std::thread(&SensorPipeline::consumerLoop, this);
}

void SensorPipeline::stop()
{
    m_running.store(false);
}

void SensorPipeline::join()
{
    if (m_producerThread.joinable())
    {
        m_producerThread.join();
    }
    if (m_consumerThread.joinable())
    {
        m_consumerThread.join();
    }
}

void SensorPipeline::producerLoop()
{
    PositionProducer producer;

    while (m_running.load())
    {
        // Your Chapter 7 concrete GPS measurement type is Position.
        const weather::Position pos = producer.nextPosition(); // keep your current name if that's what you wrote

        weather::MeasurementHeaderV1 header{};
        header.rxTime = nowNs();
        header.eventTime = header.rxTime;
        header.source = weather::SourceId::Unknown;
        header.flags = 0;
        // header.kind will be set by AnyMeasurement based on the concrete type.

        weather::AnyMeasurement msg(header, pos);

        while (m_running.load() && !m_queue.try_enqueue(msg))
        {
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void SensorPipeline::consumerLoop()
{
    PositionConsumer consumer(std::cout);

    // Scratch object: AnyMeasurement has no default ctor.
    weather::MeasurementHeaderV1 scratchHeader{};
    scratchHeader.rxTime = 0;
    scratchHeader.eventTime = 0;
    scratchHeader.source = weather::SourceId::Unknown;
    scratchHeader.flags = 0;

    weather::AnyMeasurement msg(scratchHeader, weather::Position{});

    while (m_running.load())
    {
        if (!m_queue.try_dequeue(msg))
        {
            std::this_thread::sleep_for(std::chrono::microseconds(50));
            continue;
        }

        const weather::Position* pos = msg.try_get<weather::Position>();
        if (pos != nullptr)
        {
            consumer.consume(*pos);
        }
        else
        {
            std::cout << "Unexpected measurement type\n";
        }
    }

    // Optional drain after stop (keeps output tidy)
    while (m_queue.try_dequeue(msg))
    {
        const weather::Position* pos = msg.try_get<weather::Position>();
        if (pos != nullptr)
        {
            consumer.consume(*pos);
        }
    }
}

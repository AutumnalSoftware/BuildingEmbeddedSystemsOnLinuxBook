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

// A test scaffold for the producer loop that allows for bursts of measurements,
// the idea is to _force_, from time to time, the producer to block on emplacing
// a measurement to a full queue
// TODO: It is the `PositionProducer` that is the test fake here, this `SensorPipeline`
// is supposed to be production.  Yet this test scaffold is jammed in here.  Find a
// decent way to _abstract_ this away so this "burstiness" can be removed from this
// file.
long producerEnqueueInterval()
{
    static constexpr long burstInterval = 1000;
    static constexpr long burstOf = 300;

    static long count{0};

    return count++ % burstInterval <= burstOf;
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

void SensorPipeline::status(std::ostream& os)
{
  os << "Producer enqueued: " << producer_enqueued << "\n"
     << "         blocked:  " << producer_blocked  << "\n"
     << "Consumer blocked:  " << consumer_blocked  << "\n"
     << "         dequeued: " << consumer_dequeued << "\n";
}

void SensorPipeline::producerLoop()
{
    PositionProducer producer;

    // Loop getting measurements until done (stopped)
    while (m_running.load())
    {
        const weather::Position pos = producer.nextPosition();

        weather::MeasurementHeaderV1 header{};
        header.rxTime = nowNs();
        header.eventTime = header.rxTime;
        header.source = weather::SourceId::Unknown;
        header.flags = 0;
        // header.kind will be set by AnyMeasurement based on the concrete type.

        weather::AnyMeasurement msg(header, pos);

        // busy loop until measurement successfully enqueued
        while (m_running.load())
        {
          if (m_queue.try_enqueue(msg))
          {
              producer_enqueued++;
              break;
          } else {
              producer_blocked++;
              std::this_thread::sleep_for(std::chrono::microseconds(50));
;          }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(producerEnqueueInterval()));
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
            consumer_blocked++;
            std::this_thread::sleep_for(std::chrono::microseconds(50));
            continue;
        }
        consumer_dequeued++;

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
    std::cerr << "--Draining queue--\n";
    while (true)
    {
        if (m_queue.try_dequeue(msg))
        {
            consumer_dequeued++;
            const weather::Position* pos = msg.try_get<weather::Position>();
            if (pos != nullptr)
            {
                consumer.consume(*pos);
            }
        } else {
            break;
        }
    }
}

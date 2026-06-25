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
#include "DefaultSensorPipelineIntervals.h"

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
  : SensorPipeline(capacity, getDefaultIntervals(), getDefaultIntervals())
{
}

SensorPipeline::SensorPipeline(std::size_t capacity,
                               const SensorPipelineIntervals& producerIntervals,
                               const SensorPipelineIntervals& consumerIntervals)
  : m_queue(capacity)
  , m_producerIntervals(producerIntervals)
  , m_consumerIntervals(consumerIntervals)
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
              std::this_thread::sleep_for(m_producerIntervals.blockedWaitingInterval());
          }
        }

        std::this_thread::sleep_for(m_producerIntervals.pollingInterval());
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
            std::this_thread::sleep_for(m_consumerIntervals.blockedWaitingInterval());
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

    // Drain after stop to ensure all measurements are used and
    // pipeline is emptied)
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

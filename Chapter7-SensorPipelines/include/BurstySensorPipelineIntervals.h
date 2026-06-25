#pragma once

#include <chrono>

#include "SensorPipeline.h"

// A test scaffold for the producer loop that allows for bursts of measurements,
// the idea is to _force_, from time to time, the producer to block on emplacing
// a measurement to a full queue
static
SensorPipelineIntervals& getBurstyIntervals()
{
    static constexpr long burstInterval = 1000;
    static constexpr long burstOf = 300;
    static long count {0};

    struct Bursty : public SensorPipelineIntervals
    {
        std::chrono::nanoseconds pollingInterval() const
        {
            return std::chrono::microseconds(count++ % burstInterval <= burstOf);
        }

        std::chrono::nanoseconds blockedWaitingInterval() const
        {
            return std::chrono::microseconds(50);
        }
    };

    static Bursty bursty {};
    return bursty;
}

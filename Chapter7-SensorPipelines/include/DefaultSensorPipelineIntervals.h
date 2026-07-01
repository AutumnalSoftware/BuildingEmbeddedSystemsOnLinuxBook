#pragma once

#include <chrono>

#include "SensorPipeline.h"


static
SensorPipelineIntervals& getDefaultIntervals()
{
    struct Defaults : public SensorPipelineIntervals
    {
        std::chrono::nanoseconds pollingInterval() const { return std::chrono::microseconds(1000); }
        std::chrono::nanoseconds blockedWaitingInterval() const { return std::chrono::microseconds(50); }
    };

    static Defaults defaults {};
    return defaults;
}

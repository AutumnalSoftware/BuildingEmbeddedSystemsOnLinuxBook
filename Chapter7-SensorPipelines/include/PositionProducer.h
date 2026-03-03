#pragma once

#include <cstdint>

#include "MeasurementTypes.h"

class PositionProducer
{
public:
    PositionProducer() = default;

    // Deterministic fake data generator for the demo
    weather::Position nextPosition();

private:
    std::uint32_t m_seq = 0;
};

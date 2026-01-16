#pragma once

#include <iosfwd>

#include "MeasurementTypes.h"

class PositionConsumer
{
public:
    explicit PositionConsumer(std::ostream& os);

    void consume(const weather::Position& fix);

private:
    std::ostream& m_os;
};

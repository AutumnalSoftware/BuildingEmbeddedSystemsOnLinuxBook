// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include "MeasurementTypesIO.h"
#include "PositionConsumer.h"

#include <iostream>

PositionConsumer::PositionConsumer(std::ostream& os)
    : m_os(os)
{
}

void PositionConsumer::consume(const weather::Position& pos)
{
    // This assumes you have operator<< defined for weather::Position
    // (either in MeasurementTypes.h or MeasurementTypesIO.h).
    m_os << pos << '\n';
}

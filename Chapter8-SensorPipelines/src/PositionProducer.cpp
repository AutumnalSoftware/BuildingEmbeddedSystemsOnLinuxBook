// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include "PositionProducer.h"

weather::Position PositionProducer::nextPosition()
{
    weather::Position pos;

    // Deterministic walk based on sequence number
    const double step = static_cast<double>(m_seq);

    pos.lat = 43.1566 + step * 0.0001;
    pos.lon = -77.6088 - step * 0.0001;
    pos.alt = 120.0 + step * 0.25;

    ++m_seq;
    return pos;
}

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include <iostream>

#include "AnyMeasurement.h"

int main()
{
    using namespace weather;

    // Normal measurement demo
    {
        MeasurementHeaderV1 h{};
        h.kind = MeasurementKind::Humidity; // will be overridden by ctor

        AnyMeasurement m(h, Temperature{20.0});

        std::cout << "Temperature value = "
                  << m.get<Temperature>().value << "\n";
    }

    // Empty measurement demo
    {
        MeasurementHeaderV1 h{};
        h.kind = MeasurementKind::Empty;

        AnyMeasurement e(h, Empty{});

        std::cout << "Empty measurement kind = "
                  << static_cast<int>(e.kind()) << "\n";
    }

    return 0;
}

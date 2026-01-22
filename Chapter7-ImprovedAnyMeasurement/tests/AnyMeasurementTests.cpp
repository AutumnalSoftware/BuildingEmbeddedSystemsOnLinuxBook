// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include <cassert>
#include <iostream>

#include "AnyMeasurement.h"

int main()
{
    using namespace weather;

    // Normal measurement test
    {
        MeasurementHeaderV1 h{};
        h.kind = MeasurementKind::WindSpeed; // will be overwritten by ctor

        AnyMeasurement a(h, WindSpeed{5.0});

        assert(a.kind() == MeasurementKind::WindSpeed);
        assert(a.get<WindSpeed>().value == 5.0);

        std::cout << "WindSpeed test passed\n";
    }

    // Empty measurement test
    {
        MeasurementHeaderV1 h{};
        h.kind = MeasurementKind::Empty;

        AnyMeasurement e(h, Empty{});

        assert(e.kind() == MeasurementKind::Empty);
        assert(e.try_get<WindSpeed>() == nullptr);
        assert(e.try_get<Temperature>() == nullptr);

        std::cout << "Empty test passed\n";
    }

    return 0;
}

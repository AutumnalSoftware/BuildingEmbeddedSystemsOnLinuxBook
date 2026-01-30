// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include <cassert>
#include <iostream>

#include "MutableByteView.h"

#include "BinaryWriteStream.h"

#include "NMEAStatus.h"
#include "NMEAInsertionStream.h"


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

    // Serialization smoke test (default traits: OK / no-op)
    {
        MeasurementHeaderV1 h{};
        AnyMeasurement a(h, WindSpeed{5.0});

        // NMEA stream: identifier is talker(2) + type(3)
        InsertionStream ns("WMWND");
        Status st = a.nmea_serialize(ns);
        assert(st.ok());
        std::cout << "NMEA serialization smoke test passed\n";

        std::byte buf[256]{};
        MutableByteView mv(buf, sizeof(buf));
        BinaryWriteStream bs(mv);
        a.bds_serialize(bs);
        assert(bs.ok());

        std::cout << "Binary serialization smoke test passed\n";
    }
    return 0;
}

#include "AnyMeasurement/AnyMeasurement.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>

using namespace weather;

static bool approx(double a, double b, double eps = 1e-9)
{
    return std::fabs(a - b) <= eps;
}

int main()
{
    {
        MeasurementHeaderV1 h{};
        h.rxTime = 100;
        h.eventTime = 0;
        h.kind = MeasurementKind::Temperature;
        h.source = SourceId::WeatherSensors;
        h.flags = 0xA5;

        AnyMeasurement m(h, Temperature{23.5f});
        assert(m.kind() == MeasurementKind::Temperature);
        assert(m.source() == SourceId::WeatherSensors);
        assert(m.header().rxTime == 100);
        assert(m.get<Temperature>().celsius == 23.5f);
    }

    {
        MeasurementHeaderV1 h{};
        h.rxTime = 1;
        h.kind = MeasurementKind::Precipitation;
        h.source = SourceId::WeatherSensors;

        AnyMeasurement a(h, Precipitation{1.25f, 10.0f});
        AnyMeasurement b = a;

        assert(b.kind() == MeasurementKind::Precipitation);
        assert(b.get<Precipitation>().rate_mm_per_hr == 1.25f);
        assert(b.get<Precipitation>().accumulation_mm == 10.0f);
    }

    {
        MeasurementHeaderV1 h{};
        h.rxTime = 2;
        h.eventTime = 123456;
        h.kind = MeasurementKind::Position;
        h.source = SourceId::Gps;

        AnyMeasurement a(h, Position{43.1566, -77.6088, 155.0f});
        AnyMeasurement b = std::move(a);

        assert(b.kind() == MeasurementKind::Position);
        const auto& p = b.get<Position>();
        assert(approx(p.latitude_deg, 43.1566));
        assert(approx(p.longitude_deg, -77.6088));
        assert(p.altitude_m == 155.0f);
        assert(b.header().eventTime == 123456);
    }

    {
        MeasurementHeaderV1 h{};
        h.rxTime = 42;
        h.eventTime = 1000;
        h.kind = MeasurementKind::Position;
        h.source = SourceId::Gps;
        h.flags = 0x1234;

        AnyMeasurement a(h, Position{40.7128, -74.0060, 9.0f});

        std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
        a.serialize(ss);

        ss.seekg(0);
        AnyMeasurement b = AnyMeasurement::deserialize(ss);

        assert(b.kind() == MeasurementKind::Position);
        assert(b.source() == SourceId::Gps);
        assert(b.header().rxTime == 42);
        assert(b.header().eventTime == 1000);
        assert(b.header().flags == 0x1234);

        const auto& p = b.get<Position>();
        assert(approx(p.latitude_deg, 40.7128));
        assert(approx(p.longitude_deg, -74.0060));
        assert(p.altitude_m == 9.0f);
    }

    std::cout << "All AnyMeasurement tests passed.\n";
    return 0;
}

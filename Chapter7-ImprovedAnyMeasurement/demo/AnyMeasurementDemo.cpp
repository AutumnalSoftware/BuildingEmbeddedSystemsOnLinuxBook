#include "AnyMeasurement/AnyMeasurement.h"

#include <iostream>
#include <sstream>

using namespace weather;

int main()
{
    MeasurementHeaderV1 th{};
    th.rxTime = 100;
    th.eventTime = 0;
    th.kind = MeasurementKind::Temperature;
    th.source = SourceId::WeatherSensors;

    AnyMeasurement temp(th, Temperature{21.75f});

    MeasurementHeaderV1 ph{};
    ph.rxTime = 110;
    ph.eventTime = 1700000000;
    ph.kind = MeasurementKind::Position;
    ph.source = SourceId::Gps;

    AnyMeasurement pos(ph, Position{43.1566, -77.6088, 155.0f});

    std::cout << "Temperature header: " << temp.header() << "\n";
    std::cout << "Temperature payload: " << temp.get<Temperature>().celsius << " C\n\n";

    std::cout << "Position header: " << pos.header() << "\n";
    const auto& p = pos.get<Position>();
    std::cout << "Position payload: lat=" << p.latitude_deg
              << " lon=" << p.longitude_deg
              << " alt_m=" << p.altitude_m << "\n\n";

    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    pos.serialize(ss);
    ss.seekg(0);

    AnyMeasurement pos2 = AnyMeasurement::deserialize(ss);
    const auto& p2 = pos2.get<Position>();

    std::cout << "Roundtrip deserialized header: " << pos2.header() << "\n";
    std::cout << "Roundtrip deserialized payload: lat=" << p2.latitude_deg
              << " lon=" << p2.longitude_deg
              << " alt_m=" << p2.altitude_m << "\n";

    return 0;
}

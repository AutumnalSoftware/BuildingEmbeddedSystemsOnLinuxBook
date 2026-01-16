#include <iostream>
#include "AnyMeasurement.h"

int main()
{
    using namespace weather;
    MeasurementHeaderV1 h{};
    h.kind = MeasurementKind::Humidity; // overridden
    AnyMeasurement m(h, Temperature{20.0});
    std::cout << m.get<Temperature>().value << "\n";
}

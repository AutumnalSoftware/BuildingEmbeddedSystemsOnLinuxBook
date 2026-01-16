#include <cassert>
#include <iostream>

#include "AnyMeasurement.h"

int main()
{
    using namespace weather;
    MeasurementHeaderV1 h{};
    h.kind = MeasurementKind::WindSpeed;
    AnyMeasurement a(h, WindSpeed{5.0});
    assert(a.kind() == MeasurementKind::WindSpeed);
    std::cout << "Passed" << std::endl;
    assert(a.get<WindSpeed>().value == 5.0);
    std::cout << "Passed" << std::endl;
    return 0;
}

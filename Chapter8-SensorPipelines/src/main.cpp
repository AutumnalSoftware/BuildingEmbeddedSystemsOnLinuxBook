#include <chrono>
#include <iostream>
#include <thread>

#include "PositionConsumer.h"
#include "MeasurementTypes.h"
#include "MeasurementTypesIO.h"
#include "PositionProducer.h"
#include "SensorPipeline.h"

// ----- PositionProducer -----

using namespace weather;

int main()
{
    // "Capacity" is an initial sizing parameter for the SPSC queue.
    // (We are not teaching strict boundedness in Option A.)
    SensorPipeline pipeline(/*capacity*/ 128);

    pipeline.start();

    // Run briefly
    std::this_thread::sleep_for(std::chrono::seconds(3));

    pipeline.stop();
    pipeline.join();

    return 0;
}


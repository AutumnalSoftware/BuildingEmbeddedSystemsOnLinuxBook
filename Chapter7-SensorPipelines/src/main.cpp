#include <chrono>
#include <iostream>
#include <thread>

#include "SensorPipeline.h"
#include "BurstySensorPipelineIntervals.h"
#include "DefaultSensorPipelineIntervals.h"

using namespace weather;

int main()
{
    // "Capacity" is an initial sizing parameter for the SPSC queue.
    SensorPipeline pipeline(128 /* capacity */,
                            getBurstyIntervals() /* producer intervals */,
                            getDefaultIntervals() /* consumer intervals */);

    pipeline.start();

    // Run briefly
    std::this_thread::sleep_for(std::chrono::seconds(3));

    pipeline.stop();
    pipeline.join();

    pipeline.status(std::cerr);

    return 0;
}

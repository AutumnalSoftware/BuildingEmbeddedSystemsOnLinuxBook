#include <chrono>
#include <iostream>
#include <thread>

#include "SensorPipeline.h"

using namespace weather;

int main()
{
    // "Capacity" is an initial sizing parameter for the SPSC queue.
    SensorPipeline pipeline(/*capacity*/ 128);

    pipeline.start();

    // Run briefly
    std::this_thread::sleep_for(std::chrono::seconds(3));

    pipeline.stop();
    pipeline.join();

    pipeline.status(std::cerr);

    return 0;
}

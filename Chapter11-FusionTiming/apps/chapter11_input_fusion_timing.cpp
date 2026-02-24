// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Autumnal Software

#include "WeatherSystem.h"
#include "WeatherSystemBuilder.h"

#include <chrono>
#include <iostream>
#include <thread>

int main()
{
    WeatherSystem system;
    WeatherSystemBuilder builder;

    const BuildStatus st = builder.build(system);
    if (!st)
    {
        std::cerr << "Build failed: " << st.error << " " << st.message << "\n";
        return 1;
    }

    system.start();

    // Run briefly for manual testing.
    std::this_thread::sleep_for(std::chrono::seconds(500));

    system.stop();
    system.join();

    return 0;
}

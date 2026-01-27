// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>
#include <sys/mman.h>

#include "WeatherSystem.h"
#include "WeatherSystemBuilder.h"

void lock_memory()
{
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1)
    {
        perror("mlockall");
        std::exit(EXIT_FAILURE);
    }
}

bool want_lock_memory(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--lock-memory") == 0)
            return true;
    }
    return false;
}

int main(int argc, char** argv)
{
    WeatherSystem system;
    WeatherSystemBuilder builder;

    const BuildStatus st = builder.build(system);
    if (!st)
    {
        std::cerr << "Build failed: " << st.error << " " << st.message << "\n";
        return 1;
    }

    if (want_lock_memory(argc, argv))
    {
        lock_memory(); // After construction, before execution
    }

    system.start();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    system.stop();
    system.join();

    return 0;
}

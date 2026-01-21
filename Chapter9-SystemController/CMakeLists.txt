cmake_minimum_required(VERSION 3.16)

project(Chapter9SystemDemo
    VERSION 0.1.0
    LANGUAGES CXX
)

add_executable(Chapter9SystemDemo
    main.cpp

    BuildStatus.h

    Measurements.h

    AnyMeasurement.h
    AnyMeasurement.cpp

    LogEvent.h

    Chapter9RunLoops.h
    Chapter9RunLoops.cpp

    WeatherSystem.h
    WeatherSystem.cpp

    WeatherSystemBuilder.h
    WeatherSystemBuilder.cpp
)

target_compile_features(Chapter9SystemDemo PRIVATE cxx_std_17)

target_include_directories(Chapter9SystemDemo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/readerwriterqueue
)

if (MSVC)
    target_compile_options(Chapter9SystemDemo PRIVATE /W4)
else()
    target_compile_options(Chapter9SystemDemo PRIVATE -Wall -Wextra -Wpedantic)
endif()

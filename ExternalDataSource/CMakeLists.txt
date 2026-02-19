# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Autumnal Software

cmake_minimum_required(VERSION 3.20)

project(ExternalDataSource LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(weather_tx
    main.cpp
    ITransmitEndpoint.h
    UdpTransmitEndpoint.h
    UdpTransmitEndpoint.cpp
    UartTransmitEndpoint.h
    UartTransmitEndpoint.cpp
    TransmitEndpointFactory.h
    MeasurementPacker.h
    StreamScheduler.cpp
    StreamScheduler.h
    ValueGenerator.cpp
    ValueGenerator.h
    TransmitterApp.cpp
    TransmitterApp.h
    Options.h
)

# Treat cxxopts as a header-only dependency.
add_library(cxxopts INTERFACE)
target_include_directories(cxxopts
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/../ThirdParty/cxxopts/include
        ${CMAKE_CURRENT_LIST_DIR}/../Common/include
)

target_link_libraries(weather_tx
    PRIVATE
        cxxopts
	CommonBdsMeasurementCodecs
)


add_executable(weather_rx
    receiver/WeatherReceiverApp.cpp
)

target_link_libraries(weather_rx
    PRIVATE
        CommonBdsMeasurementCodecs
        cxxopts
)
# Sensible warnings (tweak as you like).
if (MSVC)
    target_compile_options(weather_tx PRIVATE /W4)
else()
    target_compile_options(weather_tx PRIVATE -Wall -Wextra -Wpedantic)
endif()


cmake_minimum_required(VERSION 3.16)
project(chapter9_system_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(chapter9_demo
    src/main.cpp
    src/WeatherSystem.cpp
    src/WeatherSystemBuilder.cpp
    src/Chapter9RunLoops.cpp
    include/WeatherSystem.h
    include/WeatherSystemBuilder.h
    include/Chapter9MeasurementFactory.h
    include/Chapter9RunLoops.h
)

target_include_directories(chapter9_demo PRIVATE
    include
    ../Chapter6-ASCIISerialization/include
    ../Chapter7-ImprovedAnyMeasurement/include
    ../ThirdParty/readerwriterqueue
)

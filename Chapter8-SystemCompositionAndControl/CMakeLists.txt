cmake_minimum_required(VERSION 3.16)
project(chapter8_system_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(chapter8_demo
    src/main.cpp
    src/WeatherSystem.cpp
    src/WeatherSystemBuilder.cpp
    src/Chapter8RunLoops.cpp
    include/WeatherSystem.h
    include/WeatherSystemBuilder.h
    include/Chapter8RunLoops.h
)

target_include_directories(chapter8_demo PRIVATE
    include
    ../Common/include
    ../ThirdParty/readerwriterqueue
)

cmake_minimum_required(VERSION 3.16)

project(Chapter10ReactorDemo LANGUAGES CXX)

add_library(reactor STATIC
    src/SensorContext.cpp
    src/UdpSensor.cpp

    include/SensorContext.h
    include/UdpSensor.h
    include/FrameStatus.h
    include/PortStats.h
)

target_compile_features(reactor PUBLIC cxx_std_17)

target_include_directories(reactor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Adjust these include dirs to match your Chapter 10 repo layout if needed.
target_include_directories(reactor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../Chapter6-ASCIISerialization/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Chapter7-ImprovedAnyMeasurement/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty
)

add_executable(reactor_demo
    demo/main.cpp
)

target_compile_features(reactor_demo PUBLIC cxx_std_17)

target_link_libraries(reactor_demo PRIVATE reactor pthread)

target_include_directories(reactor_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty
)

target_compile_options(reactor PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(reactor_demo PRIVATE -Wall -Wextra -Wpedantic)

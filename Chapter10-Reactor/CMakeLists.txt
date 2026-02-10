cmake_minimum_required(VERSION 3.16)

project(Chapter10ReactorDemo LANGUAGES CXX)

add_library(reactor STATIC
    src/SensorContext.cpp
    src/UdpSensor.cpp
    src/UartSensor.cpp

    include/SensorContext.h
    include/UdpSensor.h
    include/UartSensor.h
    include/FrameStatus.h
    include/PortStats.h
)

target_compile_features(reactor PUBLIC cxx_std_17)

target_include_directories(reactor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Chapter6-ASCIISerialization/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Chapter7-ImprovedAnyMeasurement/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty
)

target_compile_options(reactor PRIVATE
    -Wall -Wextra -Wpedantic
)

add_executable(reactor_demo
    demo/main.cpp
)

target_compile_features(reactor_demo PUBLIC cxx_std_17)

target_include_directories(reactor_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Chapter6-ASCIISerialization/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Chapter7-ImprovedAnyMeasurement/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty
)

target_link_libraries(reactor_demo PRIVATE
    reactor
    pthread
)

target_compile_options(reactor_demo PRIVATE
    -Wall -Wextra -Wpedantic
)


# -----------------------------------------------------------------------------
# Chapter 10 system (copied and adapted from Chapter 9 pattern)
# -----------------------------------------------------------------------------
add_library(chapter10_system STATIC
    src/WeatherSystem.cpp
    src/WeatherSystemBuilder.cpp
    src/RunLoops.cpp
    src/MeasurementFactory.cpp

    include/WeatherSystem.h
    include/WeatherSystemBuilder.h
    include/RunLoops.h
    include/MeasurementFactory.h
    include/BuildStatus.h
    include/LogEvent.h
)

target_compile_features(chapter10_system PUBLIC cxx_std_17)

target_include_directories(chapter10_system PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Chapter7-ImprovedAnyMeasurement/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty
)

target_compile_options(chapter10_system PRIVATE
    -Wall -Wextra -Wpedantic
)

# -----------------------------------------------------------------------------
# Chapter 10 "real system" executable
# -----------------------------------------------------------------------------
add_executable(chapter10_integrating_external_inputs
    apps/chapter10_integrating_external_inputs.cpp
)

target_compile_features(chapter10_integrating_external_inputs PUBLIC cxx_std_17)

target_include_directories(chapter10_integrating_external_inputs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Chapter6-ASCIISerialization/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Chapter7-ImprovedAnyMeasurement/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty
)

target_link_libraries(chapter10_integrating_external_inputs PRIVATE
    reactor
    chapter10_system
    pthread
)

target_compile_options(chapter10_integrating_external_inputs PRIVATE
    -Wall -Wextra -Wpedantic
)

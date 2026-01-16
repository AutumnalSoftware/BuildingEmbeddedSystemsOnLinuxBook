cmake_minimum_required(VERSION 3.16)

project(Chapter8_SensorPipelines LANGUAGES CXX)

add_executable(Chapter8_SensorPipelines
    src/main.cpp

    # Headers explicitly listed for Qt Creator indexing
    src/GpsFix.h
    src/GpsProducer.h
    src/GpsConsumer.h
    src/SensorPipeline.h
)

target_compile_features(Chapter8_SensorPipelines PRIVATE cxx_std_17)

# Keep includes target-local and explicit.
# Use absolute paths based on this CMakeLists location.
target_include_directories(Chapter8_SensorPipelines PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/../ThirdParty/readerwriterqueue
    ${CMAKE_CURRENT_LIST_DIR}/../Chapter7-ImprovedAnyMeasurement
)

# Optional: tighten warnings (tune to your repo norms)
# target_compile_options(Chapter8_SensorPipelines PRIVATE -Wall -Wextra -Wpedantic)


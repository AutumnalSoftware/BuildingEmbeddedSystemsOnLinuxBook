cmake_minimum_required(VERSION 3.16)

project(Chapter8_SensorPipelines LANGUAGES CXX)

add_executable(SensorPipelineDemo
    src/main.cpp

    # Headers explicitly listed for Qt Creator indexing

    include/PositionProducer.h
    include/PositionConsumer.h
    include/SensorPipeline.h
    include/BurstySensorPipelineIntervals.h
    include/DefaultSensorPipelineIntervals.h
    src/SensorPipeline.cpp
    src/PositionConsumer.cpp
    src/PositionProducer.cpp
)

target_compile_features(SensorPipelineDemo PRIVATE cxx_std_17)

# Keep includes target-local and explicit.
# Use absolute paths based on this CMakeLists location.
target_include_directories(SensorPipelineDemo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/../ThirdParty/readerwriterqueue
    ${CMAKE_CURRENT_LIST_DIR}/../Common/include
)

# Optional: tighten warnings
# target_compile_options(SensorPipelineDemo PRIVATE -Wall -Wextra -Wpedantic)


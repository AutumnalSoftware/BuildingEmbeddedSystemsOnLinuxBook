cmake_minimum_required(VERSION 3.16)

project(Chapter11FusionTiming LANGUAGES CXX)

option(BUILD_DEMOS "Build demo programs" OFF)

add_library(chapter11_system STATIC
    src/WeatherSystem.cpp
    src/WeatherSystemBuilder.cpp
    src/RunLoops.cpp
    src/FusionTimingStage.cpp

    include/WeatherSystem.h
    include/WeatherSystemBuilder.h
    include/RunLoops.h
    include/FusionTimingStage.h
    include/FusionPolicy.h
    include/BuildStatus.h
    include/LogEvent.h
)

target_compile_features(chapter11_system PUBLIC cxx_std_17)

target_include_directories(chapter11_system PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Chapter7-ImprovedAnyMeasurement/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty
)

target_compile_options(chapter11_system PRIVATE
    -Wall -Wextra -Wpedantic
)

add_executable(chapter11_fusion_timing
    apps/chapter11_fusion_timing.cpp
)

target_compile_features(chapter11_fusion_timing PUBLIC cxx_std_17)

target_link_libraries(chapter11_fusion_timing PRIVATE
    chapter11_system
    pthread
)

target_compile_options(chapter11_fusion_timing PRIVATE
    -Wall -Wextra -Wpedantic
)

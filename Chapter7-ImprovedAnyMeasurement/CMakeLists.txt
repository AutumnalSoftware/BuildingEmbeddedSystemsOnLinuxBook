cmake_minimum_required(VERSION 3.20)
project(AnyMeasurementChapter7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(AnyMeasurement INTERFACE)
target_include_directories(AnyMeasurement INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_sources(AnyMeasurement INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/AnyMeasurement.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/MeasurementTypes.h
)

add_executable(anymeasurement_demo
    demo/AnyMeasurementDemo.cpp
    include/AnyMeasurement.h
    include/MeasurementTypes.h
    include/MeasurementTypesIO.h
    include/MeasurementNameOf.h
    include/MeasurementKindStrings.h
)
target_link_libraries(anymeasurement_demo PRIVATE AnyMeasurement)

add_executable(anymeasurement_tests
    tests/AnyMeasurementTests.cpp
    include/AnyMeasurement.h
    include/MeasurementTypes.h
)
target_link_libraries(anymeasurement_tests PRIVATE AnyMeasurement)

include(CTest)
add_test(NAME anymeasurement_tests COMMAND anymeasurement_tests)

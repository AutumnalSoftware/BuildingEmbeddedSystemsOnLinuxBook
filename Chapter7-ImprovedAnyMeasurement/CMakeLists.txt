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

target_include_directories(AnyMeasurement INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Chapter6-ASCIISerialization/include/nmea
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common
)

add_executable(anymeasurement_demo
    demo/AnyMeasurementDemo.cpp
    include/AnyMeasurement.h
    include/MeasurementTypes.h
    include/MeasurementTypesIO.h
    include/MeasurementNameOf.h
    include/MeasurementKindStrings.h
    include/MeasurementCodecs.h
    demo/MeasurementCodecs.cpp
)
target_link_libraries(anymeasurement_demo PRIVATE AnyMeasurement)

add_executable(anymeasurement_tests
    tests/AnyMeasurementTests.cpp
    include/AnyMeasurement.h
    include/MeasurementTypes.h
    include/MeasurementCodecs.h
)

target_include_directories(anymeasurement_tests PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../Chapter5-BinarySerialization
    ${CMAKE_CURRENT_SOURCE_DIR}/../Chapter6-ASCIISerialization/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Chapter6-ASCIISerialization/include/nmea
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common
)

target_include_directories(anymeasurement_demo PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../Chapter5-BinarySerialization
)

target_link_libraries(anymeasurement_tests PRIVATE AnyMeasurement)

add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../Chapter5-BinarySerialization
    ${CMAKE_CURRENT_BINARY_DIR}/_deps/ch5_binary
)

add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../Chapter6-ASCIISerialization
    ${CMAKE_CURRENT_BINARY_DIR}/_deps/ch6_ascii
)
target_link_libraries(anymeasurement_tests PRIVATE nmea)
target_link_libraries(anymeasurement_demo  PRIVATE nmea)   # if your demo constructs InsertionStream too


include(CTest)
add_test(NAME anymeasurement_tests COMMAND anymeasurement_tests)

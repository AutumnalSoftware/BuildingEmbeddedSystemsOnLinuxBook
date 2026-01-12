cmake_minimum_required(VERSION 3.16)

project(AnyMeasurementChapter7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ANYMEASUREMENT_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

add_library(AnyMeasurement
    include/AnyMeasurement/AnyMeasurement.h
    include/AnyMeasurement/MeasurementTypes.h
    src/MeasurementTypes.cpp
)

target_include_directories(AnyMeasurement
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (MSVC)
    target_compile_options(AnyMeasurement PRIVATE /W4)
    if (ANYMEASUREMENT_WARNINGS_AS_ERRORS)
        target_compile_options(AnyMeasurement PRIVATE /WX)
    endif()
else()
    target_compile_options(AnyMeasurement PRIVATE -Wall -Wextra -Wpedantic)
    if (ANYMEASUREMENT_WARNINGS_AS_ERRORS)
        target_compile_options(AnyMeasurement PRIVATE -Werror)
    endif()
endif()

enable_testing()

add_executable(anymeasurement_tests
    tests/AnyMeasurementTests.cpp
)
target_link_libraries(anymeasurement_tests PRIVATE AnyMeasurement)
add_test(NAME AnyMeasurementTests COMMAND anymeasurement_tests)

add_executable(anymeasurement_demo
    demo/AnyMeasurementDemo.cpp
)
target_link_libraries(anymeasurement_demo PRIVATE AnyMeasurement)

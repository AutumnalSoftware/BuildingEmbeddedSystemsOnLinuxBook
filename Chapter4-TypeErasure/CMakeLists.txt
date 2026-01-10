cmake_minimum_required(VERSION 3.16)

project(AnyMeasurementDemo
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(AnyMeasurementDemo
    main.cpp
)

# Helpful warnings, but keep them readable for the book
if (MSVC)
    target_compile_options(AnyMeasurementDemo PRIVATE /W4)
else()
    target_compile_options(AnyMeasurementDemo PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()


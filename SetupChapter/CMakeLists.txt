cmake_minimum_required(VERSION 3.16)

# Project name and version
project(Chapter2MemoryTools VERSION 1.0 LANGUAGES CXX)

# Require C++17 (book's minimum version)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Enable warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror=return-type)
endif()

add_executable(mlock_demo_program
    mlock_demo_program.cpp
)

add_executable(jitter_test jitter_test.cpp)
target_compile_options(jitter_test PRIVATE -O2)

# Linux-specific: mlockall needs real-time library on some distros
# (Not always needed, but harmless if present.)
if (UNIX AND NOT APPLE)
    target_link_libraries(mlock_demo_program PRIVATE rt)
endif()


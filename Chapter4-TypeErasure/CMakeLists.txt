cmake_minimum_required(VERSION 3.16)

project(TypeErasureDemo
    VERSION 0.1.0
    LANGUAGES CXX
)

# ---- Language standard ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Executable ----
add_executable(TypeErasureDemo
    main.cpp
    AnyNmeaMessage.h
)

# ---- Include current directory for headers ----
target_include_directories(TypeErasureDemo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# ---- Warnings (reasonable defaults) ----
if (MSVC)
    target_compile_options(TypeErasureDemo PRIVATE /W4)
else()
    target_compile_options(TypeErasureDemo PRIVATE -Wall -Wextra -Wpedantic)
endif()


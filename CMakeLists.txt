cmake_minimum_required(VERSION 3.22)

project(AutumnalWeatherSystem
    VERSION 1.0
    DESCRIPTION "Designing Deterministic Systems with Modern C++ - Book Code"
    LANGUAGES CXX
)

# ------------------------------------------------------------
# Global C++ Settings
# ------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Strong warnings (GCC/Clang)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
    )
endif()

# ------------------------------------------------------------
# Optional: Enable folders in IDEs like Qt Creator / CLion
# ------------------------------------------------------------

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ------------------------------------------------------------
# Add Chapters
# ------------------------------------------------------------

add_subdirectory(Common)
add_subdirectory(Chapter4-TypeErasure)
add_subdirectory(Chapter8-SensorPipelines)
add_subdirectory(Chapter9-SystemController)
add_subdirectory(Chapter10-Reactor)


option(BUILD_DEMOS "Build demo programs" OFF)

# -----------------------------------------------------------------------------
# Chapter 12 BDS reactor (UDP only, first milestone)
# -----------------------------------------------------------------------------

add_library(chapter12_reactor_bds STATIC
    src/SensorContext.cpp
    src/UdpSensor.cpp

    include/SensorContext.h
    include/UdpSensor.h
    include/FrameStatus.h
    include/PortStats.h
)

target_compile_features(chapter12_reactor_bds PUBLIC cxx_std_17)

target_include_directories(chapter12_reactor_bds PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty
)

target_compile_options(chapter12_reactor_bds PRIVATE
    -Wall -Wextra -Wpedantic
)

target_link_libraries(chapter12_reactor_bds PUBLIC
    CommonBdsMeasurementCodecs
)

# -----------------------------------------------------------------------------
# Chapter 12 system (Chapter 11 pattern)
# -----------------------------------------------------------------------------

add_library(chapter12_system STATIC
    src/FusionState.cpp
    src/WeatherSystem.cpp
    src/WeatherSystemBuilder.cpp
    src/RunLoops.cpp

    include/WeatherSystem.h
    include/WeatherSystemBuilder.h
    include/RunLoops.h
    include/BuildStatus.h
    include/LogEvent.h
    include/FusionState.h
    include/FusedWeatherSample.h
    include/SystemStats.h
)

target_compile_features(chapter12_system PUBLIC cxx_std_17)

target_include_directories(chapter12_system PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty
)

target_compile_options(chapter12_system PRIVATE
    -Wall -Wextra -Wpedantic
)

target_link_libraries(chapter12_system PRIVATE
    chapter12_reactor_bds
    Common
)

# -----------------------------------------------------------------------------
# Chapter 12 executable
# -----------------------------------------------------------------------------

add_executable(chapter12_deterministic_fusion_and_output
    apps/chapter12_deterministic_fusion_and_output.cpp
)

target_compile_features(chapter12_deterministic_fusion_and_output PUBLIC cxx_std_17)

target_link_libraries(chapter12_deterministic_fusion_and_output PRIVATE
    chapter12_system
    pthread
)

target_compile_options(chapter12_deterministic_fusion_and_output PRIVATE
    -Wall -Wextra -Wpedantic
)

if (BUILD_DEMOS)
    # No demos yet for Chapter 12.
endif()

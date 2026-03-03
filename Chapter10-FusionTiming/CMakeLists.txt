option(BUILD_DEMOS "Build demo programs" OFF)

# -----------------------------------------------------------------------------
# Chapter 11 BDS reactor (UDP only, first milestone)
# -----------------------------------------------------------------------------

add_library(chapter11_reactor_bds STATIC
    src/SensorContext.cpp
    src/UdpSensor.cpp

    include/SensorContext.h
    include/UdpSensor.h
    include/FrameStatus.h
    include/PortStats.h
)

target_compile_features(chapter11_reactor_bds PUBLIC cxx_std_17)

target_include_directories(chapter11_reactor_bds PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty
)

target_compile_options(chapter11_reactor_bds PRIVATE
    -Wall -Wextra -Wpedantic
)

target_link_libraries(chapter11_reactor_bds PUBLIC
    CommonBdsMeasurementCodecs
)

# -----------------------------------------------------------------------------
# Chapter 11 system (Chapter 10 pattern)
# -----------------------------------------------------------------------------

add_library(chapter11_system STATIC
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
    include/SystemStats.h
)

target_compile_features(chapter11_system PUBLIC cxx_std_17)

target_include_directories(chapter11_system PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty
)

target_compile_options(chapter11_system PRIVATE
    -Wall -Wextra -Wpedantic
)

target_link_libraries(chapter11_system PRIVATE
    chapter11_reactor_bds
    Common
)

# -----------------------------------------------------------------------------
# Chapter 11 executable
# -----------------------------------------------------------------------------

add_executable(chapter11_input_fusion_timing
    apps/chapter11_input_fusion_timing.cpp
    src/FusionState.cpp
)

target_compile_features(chapter11_input_fusion_timing PUBLIC cxx_std_17)

target_link_libraries(chapter11_input_fusion_timing PRIVATE
    chapter11_system
    pthread
)

target_compile_options(chapter11_input_fusion_timing PRIVATE
    -Wall -Wextra -Wpedantic
)

if (BUILD_DEMOS)
    # No demos yet for Chapter 11.
endif()

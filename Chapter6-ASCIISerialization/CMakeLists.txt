cmake_minimum_required(VERSION 3.16)
project(ch6_nmea LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(nmea
    src/NMEATokenizer.cpp
    src/NMEAExtractionStream.cpp
    src/NMEAInsertionStream.cpp

    include/NMEAStatus.h
    include/NMEATokenizer.h
    include/NMEAExtractionStream.h
    include/NMEAInsertionStream.h
)

target_include_directories(nmea PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(nmea_demo examples/nmea_demo.cpp)
target_link_libraries(nmea_demo PRIVATE nmea)

add_executable(nmea_tests tests/test_nmea.cpp)
target_link_libraries(nmea_tests PRIVATE nmea)

enable_testing()
add_test(NAME nmea_tests COMMAND nmea_tests)

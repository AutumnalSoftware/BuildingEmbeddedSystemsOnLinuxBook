cmake_minimum_required(VERSION 3.16)
project(TypeErasureChapter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SHARED_SOURCES
    AnyNMEAMessage.h
    NMEACommon.cpp
    NMEACommon.h
    NMEAExtractionStream.cpp
    NMEAExtractionStream.h
    NMEAInsertionStream.cpp
    NMEAInsertionStream.h
)

add_executable(typeErasureDemo
    typeErasureDemo.cpp
    ${SHARED_SOURCES}
)

add_executable(typeErasureTests
    tests.cpp
    ${SHARED_SOURCES}
)

target_include_directories(typeErasureDemo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

target_include_directories(typeErasureTests PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

# If AnyNMEAMessage is header-only, nothing else needed.
# If you later add AnyNMEAMessage.cpp, add it to SHARED_SOURCES.

include(CTest)
add_test(NAME typeErasureTests COMMAND typeErasureTests)


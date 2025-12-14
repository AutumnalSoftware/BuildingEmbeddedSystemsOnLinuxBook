cmake_minimum_required(VERSION 3.16)
project(TypeErasureChapter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SHARED_SOURCES
    ImmutableBuffer.cpp
    MutableBuffer.cpp
    NMEACommon.cpp
    NMEAExtractionStream.cpp
    NMEAInsertionStream.cpp
)

add_executable(typeErasureDemo
    typeErasureDemo.cpp
    ${SHARED_SOURCES}
)

add_executable(typeErasureTests
    tests.cpp
    ${SHARED_SOURCES}
)

# If AnyNMEAMessage is header-only, nothing else needed.
# If you later add AnyNMEAMessage.cpp, add it to SHARED_SOURCES.

include(CTest)
add_test(NAME typeErasureTests COMMAND typeErasureTests)


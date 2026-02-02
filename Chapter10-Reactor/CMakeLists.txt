cmake_minimum_required(VERSION 3.16)
project(reactor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#add_library(reactor
#    #/include/SensorContext.h
    #/src/context.cpp
#)

#target_include_directories(reactor PUBLIC
    #${CMAKE_CURRENT_SOURCE_DIR}/include
    #${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty
    #${CMAKE_CURRENT_SOURCE_DIR}/../Chapter5-BinarySerialization/include
    #${CMAKE_CURRENT_SOURCE_DIR}/../Chapter6-ASCIISerialization/include
    #${CMAKE_CURRENT_SOURCE_DIR}/../Chapter7-ImprovedAnyMeasurement/include
#)

#add_executable(reactor_demo examples/reactor_demo.cpp)
#target_link_libraries(reactor_demo PRIVATE reactor)

#add_executable(reactor_tests tests/test_reactor.cpp)
#target_link_libraries(reactor_tests PRIVATE reactor)

add_executable(epollPrimer epollPrimer/epollPrimer.cpp)
#target_link_libraries(epollPrimer PRIVATE reactor)

#enable_testing()
#add_test(NAME reactor_tests COMMAND reactor_tests)

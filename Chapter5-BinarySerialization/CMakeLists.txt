cmake_minimum_required(VERSION 3.16)

project(TestBdsSerialization LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(TestBdsSerialization 
    main.cpp)

target_include_directories(TestBdsSerialization
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../Common
)

include(GNUInstallDirs)
install(TARGETS TestBdsSerialization
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

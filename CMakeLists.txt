cmake_minimum_required(VERSION 3.13)

project(scheme-cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${CMAKE_SOURCE_DIR})

file(GLOB SOURCES src/*.cpp)

add_executable(scheme
    ${SOURCES}
    main.cpp
)
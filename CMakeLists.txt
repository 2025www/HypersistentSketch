cmake_minimum_required(VERSION 3.14)
project(bench)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -msse2 -mssse3 -msse4.1 -msse4.2 -mavx -mavx2 -mbmi")

include_directories(.)
include_directories(common)
include_directories(Others)
include_directories(HypersistentSketch)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

add_executable(bench
        main.cpp
)

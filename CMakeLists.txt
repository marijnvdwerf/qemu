cmake_minimum_required(VERSION 3.3)
project(qemu_dev)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

file(GLOB_RECURSE src "*.c" "*.h")

set(SOURCES ${src})
add_library(qemu_dev ${SOURCES})
target_include_directories(qemu_dev PUBLIC "include" "target/arm" "")

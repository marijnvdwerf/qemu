cmake_minimum_required(VERSION 3.3)
project(qemu_dev)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

file(GLOB_RECURSE src "*.c" ".h")

set(SOURCES
    ${src})


add_executable(qemu_dev ${SOURCES})
target_compile_options(qemu_dev PRIVATE -iquote "${CMAKE_CURRENT_SOURCE_DIR}/include")
#target_include_directories(qemu_dev PRIVATE include)


cmake_minimum_required(VERSION 2.18)

project(sif C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

add_subdirectory(third_party)
add_subdirectory(sif)
add_subdirectory(mv-svole)
add_subdirectory(test)


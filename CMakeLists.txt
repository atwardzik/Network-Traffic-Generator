cmake_minimum_required(VERSION 3.30)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 23)

project(network_traffic_generator)

add_subdirectory(libc)
add_subdirectory(backend)
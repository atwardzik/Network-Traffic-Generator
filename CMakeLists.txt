cmake_minimum_required(VERSION 3.30)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 23)

project(network_traffic_generator)

add_compile_options(-Wall -Wextra -Werror -Wno-incompatible-library-redeclaration)

add_subdirectory(libc)
add_subdirectory(backend)
add_subdirectory(generator)
add_subdirectory(scanner)
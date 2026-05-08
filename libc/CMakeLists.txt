add_library(libc
    libc.c
)

target_include_directories(libc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
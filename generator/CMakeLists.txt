add_executable(generator
        src/modbustcp.c
)

target_include_directories(generator PRIVATE include)

target_link_libraries(generator PRIVATE libc)
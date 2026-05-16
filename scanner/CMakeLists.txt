add_executable(scanner
        src/main.c
        src/scanning.c
)

target_include_directories(scanner PRIVATE include)

target_link_libraries(scanner PRIVATE libc)
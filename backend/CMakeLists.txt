add_executable(backend
        src/main.c
        src/server.c
)

target_include_directories(backend PRIVATE include)

target_link_libraries(backend PRIVATE libc)
add_executable(backend_test
        test_server.c
)

target_include_directories(backend_test PRIVATE include)

target_link_libraries(backend_test PRIVATE libc)
add_library(libc
        libc.c
)

set_source_files_properties(libc.c PROPERTIES COMPILE_FLAGS -Wno-incompatible-pointer-types-discards-qualifiers)
target_include_directories(libc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
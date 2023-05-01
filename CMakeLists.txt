cmake_minimum_required(VERSION 3.24)
project(space_game)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
        -Wall
        -Wextra
        -Wconversion
        -Wsign-conversion
        $<$<CONFIG:DEBUG>:-g3>
        $<$<CONFIG:DEBUG>:-Og>
        $<$<CONFIG:RELEASE>:-O3>)

add_compile_definitions($<$<CONFIG:DEBUG>:_DEBUG>)

if (WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif ()

add_subdirectory(engine)
add_subdirectory(testbed)
add_subdirectory(game)
add_subdirectory(assets)

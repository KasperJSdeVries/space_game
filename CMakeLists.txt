cmake_minimum_required(VERSION 3.24)
project(space_game)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(engine)
add_subdirectory(testbed)
add_subdirectory(game)

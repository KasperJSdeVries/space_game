cmake_minimum_required(VERSION 3.24)
project(space_game)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(CMAKE_BUILD_TYPE EQUAL "DEBUG")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Wextra")
endif()

if(WIN32)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

add_subdirectory(engine)
add_subdirectory(testbed)
add_subdirectory(game)

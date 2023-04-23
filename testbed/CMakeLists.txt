cmake_minimum_required(VERSION 3.24)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCE_FILES src/**.c)

add_executable(testbed ${SOURCE_FILES})

target_include_directories(testbed PUBLIC ${SPACE_ENGINE_INCLUDE_DIRS})

target_link_libraries(testbed space_engine)

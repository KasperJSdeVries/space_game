cmake_minimum_required(VERSION 3.24)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCE_FILES src/**.c)

add_executable(space_game ${SOURCE_FILES})

target_include_directories(space_game PUBLIC ${SPACE_ENGINE_INCLUDE_DIRS})

target_link_libraries(space_game PUBLIC space_engine)

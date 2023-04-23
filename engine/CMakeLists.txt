cmake_minimum_required(VERSION 3.24)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan 1.3 REQUIRED)

set(SPACE_ENGINE_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    CACHE PATH "Path to space engine include directories" FORCE)
set(SPACE_ENGINE_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    CACHE PATH "Path to space engine source directory" FORCE)

file(GLOB_RECURSE SOURCE_FILES src/**.c)

add_library(space_engine SHARED ${SOURCE_FILES})

target_include_directories(
  space_engine
  PUBLIC include
  PRIVATE src
  PRIVATE ${VULKAN_INCLUDE_DIRS})

target_link_libraries(space_engine ${VULKAN_LIB_LIST})

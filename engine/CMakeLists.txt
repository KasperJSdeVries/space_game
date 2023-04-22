cmake_minimum_required(VERSION 3.24)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)

set(SOURCE_FILES src/core/logger.c)
set(SPACE_ENGINE_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    CACHE PATH "Path to space engine include directory" FORCE)

add_library(space_engine SHARED ${SOURCE_FILES})

target_include_directories(
  space_engine
  PUBLIC include
  PRIVATE src
  PRIVATE ${GLFW_INCLUDE_DIRS}
  PRIVATE ${VULKAN_INCLUDE_DIRS})

target_link_libraries(space_engine glfw)
target_link_libraries(space_engine ${VULKAN_LIB_LIST})

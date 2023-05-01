cmake_minimum_required(VERSION 3.24)

find_package(Vulkan REQUIRED)

set(ASSETS_OUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets")

set(SHADERS_IN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(SHADERS_OUT_DIR "${ASSETS_OUT_DIR}/shaders")

file(GLOB_RECURSE SHADERS "${SHADERS_IN_DIR}/*.vert" "${SHADERS_IN_DIR}/*.frag")

if(NOT ${Vulkan_glslangValidator_FOUND})
  message(SEND_ERROR "Could not find glslangValidator")
endif()

foreach(SHADER ${SHADERS})
  get_filename_component(SHADER_NAME ${SHADER} NAME)
  set(SHADER_OUT_NAME "${SHADERS_OUT_DIR}/${SHADER_NAME}.spv")
  list(APPEND SHADER_OUT_NAMES ${SHADER_OUT_NAME})
  add_custom_command(
    MAIN_DEPENDENCY ${SHADER}
    OUTPUT ${SHADER_OUT_NAME}
    COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} "-V" "-o" ${SHADER_OUT_NAME}
            ${SHADER}
    VERBATIM)
endforeach()

add_custom_target(build_shaders DEPENDS ${SHADER_OUT_NAMES})

add_custom_command(
  TARGET build_shaders
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}"
          "${ASSETS_OUT_DIR}"
  VERBATIM)

add_dependencies(space_engine build_shaders)

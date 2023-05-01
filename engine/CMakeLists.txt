cmake_minimum_required(VERSION 3.24)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan 1.3 REQUIRED)
if (UNIX AND NOT APPLE)
    set(LINUX TRUE)

    find_package(ECM REQUIRED NO_MODULE)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

    find_package(X11 REQUIRED)
    find_package(XCB REQUIRED)
    find_package(X11_XCB REQUIRED)
endif ()

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
        PRIVATE ${Vulkan_INCLUDE_DIRS})

if (LINUX)
    target_include_directories(
            space_engine
            PRIVATE ${X11_INCLUDE_DIR}
            PRIVATE ${XCB_INCLUDE_DIR}
            PRIVATE ${X11_XCB_INCLUDE_DIR})
endif ()

target_link_libraries(space_engine ${Vulkan_LIBRARIES})

if (LINUX)
    target_link_libraries(space_engine ${X11_LIBRARIES} ${XCB_LIBRARIES}
            ${X11_XCB_LIBRARIES})
endif ()

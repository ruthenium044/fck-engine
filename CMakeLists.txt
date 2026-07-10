# fck
cmake_minimum_required(VERSION 3.26)

project(fck)

# each module should take care of this...
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_MODULE_LIBRARY_PREFIX "")

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

set(fck_static OFF) 
if(fck_static) 
    add_compile_definitions(fck_static_export)
    set(BUILD_SHARED_LIBS OFF)
else()
    set(BUILD_SHARED_LIBS ON)
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_sources(${PROJECT_NAME} PRIVATE ".vs/launch.vs.json")

add_subdirectory(input-all)
add_subdirectory(input-mouse)
add_subdirectory(input-keyboard)
add_subdirectory(input-dualsense)
add_subdirectory(input-text)
add_subdirectory(render-vk)
add_subdirectory(shader)
add_subdirectory(app)
add_subdirectory(png)
add_subdirectory(glsl-reflection)
add_subdirectory(gfx)
add_subdirectory(nuklear)
add_subdirectory(sprite)
add_subdirectory(api)
add_subdirectory(plugins)
add_subdirectory(ec)

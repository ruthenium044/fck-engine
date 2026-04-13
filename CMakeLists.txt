# fck
cmake_minimum_required(VERSION 3.26)

project(fck)

# each module should take care of this...
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_MODULE_LIBRARY_PREFIX "")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

set(fck_static OFF) 
if(fck_static) 
    add_compile_definitions(fck_static_export)
    set(BUILD_SHARED_LIBS OFF)
else()
    set(BUILD_SHARED_LIBS ON)
endif()

#set(FCK_WORKSPACE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "Workspace Path")
#list(APPEND CMAKE_PREFIX_PATH "${FCK_WORKSPACE_PATH}" "..")

add_library(${PROJECT_NAME} INTERFACE)
target_sources(${PROJECT_NAME} PRIVATE ".vs/launch.vs.json")

add_subdirectory(render-vk)

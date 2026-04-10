# fck
cmake_minimum_required(VERSION 3.26)

project(fck)

# each module should take care of this...
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_MODULE_LIBRARY_PREFIX "")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

set(FCK_WORKSPACE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "Workspace Path")
list(APPEND CMAKE_PREFIX_PATH "${FCK_WORKSPACE_PATH}" "..")

add_subdirectory(render-vk)

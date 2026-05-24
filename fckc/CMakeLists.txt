# fckc
cmake_minimum_required(VERSION 3.20)
include_guard(GLOBAL)

project(fckc)

add_library(${PROJECT_NAME} INTERFACE)
add_library(fck::fckc ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_sources(${PROJECT_NAME} PRIVATE "include/fckc_apidef.h")
target_sources(${PROJECT_NAME} PRIVATE "include/fckc_assert.h")
target_sources(${PROJECT_NAME} PRIVATE "include/fckc_atomic.h")
target_sources(${PROJECT_NAME} PRIVATE "include/fckc_inttypes.h")
target_sources(${PROJECT_NAME} PRIVATE "include/fckc_math.h")

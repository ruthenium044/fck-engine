cmake_minimum_required(VERSION 3.20)

include_guard(GLOBAL)

project(fck-gameloop)

add_library(${PROJECT_NAME} INTERFACE)
add_library(fck::gameloop ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_sources(${PROJECT_NAME} PRIVATE "include/fck_gameloop.h")
# FCK HASH
cmake_minimum_required(VERSION 3.20)

project(fck-hash)

include_guard(GLOBAL)

add_executable(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_sources(${PROJECT_NAME} PRIVATE "include/fck_hash.h")
target_sources(${PROJECT_NAME} PRIVATE "src/main.c")

add_library(${PROJECT_NAME}-interface INTERFACE include/fck_hash.h)
add_library(fck::hash ALIAS ${PROJECT_NAME}-interface)

target_include_directories(${PROJECT_NAME}-interface INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_sources(${PROJECT_NAME}-interface PRIVATE "include/fck_hash.h")
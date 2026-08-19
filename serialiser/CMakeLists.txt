cmake_minimum_required(VERSION 3.20)

project(fck-serialiser)

include_guard(GLOBAL)

add_library(${PROJECT_NAME} INTERFACE)
add_library(fck::serialiser ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_sources(${PROJECT_NAME} INTERFACE "include/fck_serialiser.h")

find_package(fckc)
target_link_libraries(${PROJECT_NAME} INTERFACE fck::fckc)

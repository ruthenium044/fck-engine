# render
cmake_minimum_required(VERSION 3.20)

project(fck-render)

include_guard(GLOBAL)

add_library(${PROJECT_NAME} INTERFACE)
add_library(fck::render ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_sources(${PROJECT_NAME} INTERFACE "include/sht_render.h")

find_package(fckc)
target_link_libraries(${PROJECT_NAME} INTERFACE fck::fckc)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-headers")


# fckc
cmake_minimum_required(VERSION 3.20)

project(fck-imgui)

include_guard(GLOBAL)

add_library(${PROJECT_NAME} INTERFACE "include/fck_imgui.h")
add_library(fck::imgui ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(fckc)
target_link_libraries(${PROJECT_NAME} INTERFACE fck::fckc)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-headers")


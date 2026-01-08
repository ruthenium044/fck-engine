# fckc
cmake_minimum_required(VERSION 3.20)

project(fck-app-loadable)

include_guard(GLOBAL)

add_library(${PROJECT_NAME} INTERFACE "include/fck_app_loadable.h")
add_library(fck::app-loadable ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-headers")


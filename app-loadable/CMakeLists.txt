# fckc
cmake_minimum_required(VERSION 3.20)

project(fck-app-loadable)

include_guard(GLOBAL)

add_library(${PROJECT_NAME} INTERFACE)
add_library(fck::app-loadable ALIAS ${PROJECT_NAME})

file(GLOB_RECURSE FCK_APP_LOADABLE_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

add_custom_target(${PROJECT_NAME}-headers SOURCES ${FCK_APP_LOADABLE_HEADERS})
add_dependencies(${PROJECT_NAME}-headers fck::app-loadable)

target_include_directories(${PROJECT_NAME} INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>"
)

set_target_properties(${PROJECT_NAME}-headers PROPERTIES FOLDER "fck-lib")


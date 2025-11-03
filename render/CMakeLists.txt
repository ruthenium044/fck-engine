# fckc
cmake_minimum_required(VERSION 3.20)

project(fck-render)

include_guard(GLOBAL)

add_library(${PROJECT_NAME} INTERFACE)
add_library(fck::render ALIAS ${PROJECT_NAME})

file(GLOB_RECURSE FCKC_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

add_custom_target(${PROJECT_NAME}-headers SOURCES ${FCKC_HEADERS})
add_dependencies(${PROJECT_NAME}-headers fck::render)

target_include_directories(${PROJECT_NAME} INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>"
)

find_package(img CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
find_package(fckc CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
target_link_libraries(${PROJECT_NAME} INTERFACE fck::fckc fck::img)

set_target_properties(${PROJECT_NAME}-headers PROPERTIES FOLDER "fck-lib")


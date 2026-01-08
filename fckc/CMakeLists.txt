# fckc
cmake_minimum_required(VERSION 3.20)

project(fckc)

include_guard(GLOBAL)

file(GLOB_RECURSE FCKC_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

add_library(${PROJECT_NAME} INTERFACE ${FCKC_HEADERS})
add_library(fck::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-headers")


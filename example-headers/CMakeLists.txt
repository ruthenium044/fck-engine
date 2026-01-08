# example
cmake_minimum_required(VERSION 3.20)

project(example-headers)

include_guard(GLOBAL)

add_library(${PROJECT_NAME} INTERFACE "include/fck_example_header.h")
add_library(fck::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} INTERFACE
    include
)

set_target_properties(${PROJECT_NAME}-headers PROPERTIES FOLDER "fck-lib")


# FCK HASH
cmake_minimum_required(VERSION 3.20)

project(fck-hash)

include_guard(GLOBAL)

include_directories(${PROJECT_NAME} include)

add_executable(${PROJECT_NAME} main.c include/fck_hash.h)
target_sources(${PROJECT_NAME}
    PRIVATE main.c
    PRIVATE include/fck_hash.h
)

add_library(${PROJECT_NAME}-headers INTERFACE include/fck_hash.h)
add_library(fck::hash ALIAS ${PROJECT_NAME}-headers)
target_include_directories(${PROJECT_NAME}-headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-exe")
set_target_properties(${PROJECT_NAME}-headers PROPERTIES FOLDER "fck-headers")

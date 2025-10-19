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

add_library(${PROJECT_NAME}-lib INTERFACE)
add_library(fck::hash ALIAS ${PROJECT_NAME}-lib)

add_custom_target(${PROJECT_NAME}-headers SOURCES include/fck_hash.h)
add_dependencies(${PROJECT_NAME}-headers fck::hash)

target_include_directories(${PROJECT_NAME}-lib INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>"
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-exe")
set_target_properties(${PROJECT_NAME}-headers PROPERTIES FOLDER "fck-lib")

# FCK HASH
cmake_minimum_required(VERSION 3.20)

project(hash)

include_guard(GLOBAL)

include_directories(${PROJECT_NAME} include)

add_executable(fck-${PROJECT_NAME} main.c include/fck_hash.h)
target_sources(fck-${PROJECT_NAME}
    PRIVATE main.c
    PRIVATE include/fck_hash.h
)

add_library(fck-${PROJECT_NAME}-lib INTERFACE)
add_library(fck::${PROJECT_NAME} ALIAS fck-${PROJECT_NAME}-lib)

add_custom_target(fck-${PROJECT_NAME}-headers SOURCES include/fck_hash.h)
add_dependencies(fck-${PROJECT_NAME}-headers fck::${PROJECT_NAME})

target_include_directories(fck-${PROJECT_NAME}-lib INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>"
)

cmake_minimum_required(VERSION 3.20)

include_guard(GLOBAL)

project(fck-api)

find_package(hash)
find_package(fckc)

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}-interface INTERFACE)

add_library(fck::api ALIAS ${PROJECT_NAME})
add_library(fck::api-interface ALIAS ${PROJECT_NAME}-interface)

target_include_directories(${PROJECT_NAME}-interface INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(${PROJECT_NAME}-interface INTERFACE fck::fckc fck::hash)
target_sources(${PROJECT_NAME}-interface PRIVATE "include/fck_apis.h")

target_sources(${PROJECT_NAME} PRIVATE "src/fck_apis.c")

target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}-interface)
target_link_libraries(${PROJECT_NAME}-interface INTERFACE fck::fckc)

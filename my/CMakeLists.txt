cmake_minimum_required(VERSION 3.20)

project(my)

include_guard(GLOBAL)

find_package(fckc)
find_package(os)
find_package(api)
find_package(kll)

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}-interface INTERFACE)

add_library(fck::my ALIAS ${PROJECT_NAME})
add_library(fck::my-interface ALIAS ${PROJECT_NAME}-interface)

target_include_directories(${PROJECT_NAME}-interface INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_sources(${PROJECT_NAME}-interface PRIVATE "include/my.h")

target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}-interface)

target_include_directories(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_sources(${PROJECT_NAME} PRIVATE "src/my.c")

target_link_libraries(${PROJECT_NAME}-interface INTERFACE fck::fckc)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::os fck::kll fck::api-interface)

cmake_minimum_required(VERSION 3.20)

project(fck-png)

include_guard(GLOBAL)

include(FetchContent)

find_package(fckc)
find_package(os)
find_package(api)

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}-interface INTERFACE)

add_library(fck::png ALIAS ${PROJECT_NAME})
add_library(fck::png-interface ALIAS ${PROJECT_NAME}-interface)

target_include_directories(${PROJECT_NAME}-interface INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(${PROJECT_NAME}-interface INTERFACE fck::fckc)
target_sources(${PROJECT_NAME}-interface PRIVATE "include/fck_png.h")

target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}-interface)

target_include_directories(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_sources(${PROJECT_NAME} PRIVATE "src/stb_image.h")
target_sources(${PROJECT_NAME} PRIVATE "src/fck_png.c")

target_link_libraries(${PROJECT_NAME} PUBLIC fck::os fck::api-interface)

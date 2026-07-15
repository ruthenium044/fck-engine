# fckc
cmake_minimum_required(VERSION 3.20)

include_guard(GLOBAL)

project(fck-db)

find_package(api)
find_package(os)
find_package(hash)
find_package(kll)
find_package(fckc)

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}-interface INTERFACE)

add_library(fck::db ALIAS ${PROJECT_NAME})
add_library(fck::db-interface ALIAS ${PROJECT_NAME}-interface)

target_include_directories(${PROJECT_NAME}-interface INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_sources(${PROJECT_NAME}-interface PRIVATE "include/fck_db.h")
target_sources(${PROJECT_NAME} PRIVATE "src/fck_db.c")

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}-interface)
target_link_libraries(${PROJECT_NAME}-interface INTERFACE fck::fckc)

target_link_libraries(${PROJECT_NAME} PRIVATE fck::hash)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::os)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::kll)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::api-interface)
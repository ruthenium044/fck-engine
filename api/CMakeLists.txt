cmake_minimum_required(VERSION 3.20)

project(fck-api)

include_guard(GLOBAL)

add_library(${PROJECT_NAME})
add_library(fck::api ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

target_sources(${PROJECT_NAME} PRIVATE "include/fck_apis.h")
target_sources(${PROJECT_NAME} PRIVATE "src/fck_apis.c")

find_package(hash)
find_package(fckc)
find_package(kll)
find_package(std)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::fckc PRIVATE fck::kll fck::hash fck::std)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-lib")

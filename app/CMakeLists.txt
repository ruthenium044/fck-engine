# fckc
cmake_minimum_required(VERSION 3.20)

include_guard(GLOBAL)

project(fck-app)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE "main.c")

find_package(api)
find_package(os)
find_package(hash)
find_package(input)

target_link_libraries(${PROJECT_NAME} PRIVATE fck::hash fck::os fck::input fck::api)

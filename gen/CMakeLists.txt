cmake_minimum_required(VERSION 3.20)

project(fck-gen)

find_package(fckc)
find_package(os)
find_package(kll)

add_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE fck::fckc fck::os fck::kll)

target_sources(${PROJECT_NAME} PRIVATE "main.c")

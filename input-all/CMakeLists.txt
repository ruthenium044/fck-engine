cmake_minimum_required(VERSION 3.20)

project(fck-input-all)

include_guard(GLOBAL)

add_library(${PROJECT_NAME})
add_library(fck::input-all ALIAS ${PROJECT_NAME})

find_package(fckc)
find_package(input)
find_package(os)
find_package(api)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::fckc fck::input fck::os fck::api-headers)

target_sources(${PROJECT_NAME} PRIVATE "src/fck_input.c")

add_executable(${PROJECT_NAME}-test "test/main.c")
target_link_libraries(${PROJECT_NAME}-test PRIVATE ${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-lib")
set_target_properties(${PROJECT_NAME}-test PROPERTIES FOLDER "fck-tests")
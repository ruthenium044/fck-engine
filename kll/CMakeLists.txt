# KiLLer Allocators
cmake_minimum_required(VERSION 3.20)

project(fck-kll)

include_guard(GLOBAL)

add_library(${PROJECT_NAME})
add_library(fck::kll ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_sources(${PROJECT_NAME} PRIVATE "include/kll.h")

target_sources(${PROJECT_NAME} PRIVATE "include/kll_system.h")
target_sources(${PROJECT_NAME} PRIVATE "include/kll_temp.h")

target_sources(${PROJECT_NAME} PRIVATE "include/kll_malloc.h")

target_sources(${PROJECT_NAME} PRIVATE "src/kll.c")
target_sources(${PROJECT_NAME} PRIVATE "src/kll_system.c")
target_sources(${PROJECT_NAME} PRIVATE "src/kll_temp.c")

find_package(fckc)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::fckc)
#target_compile_definitions(${PROJECT_NAME} PRIVATE FCK_BUILD_DLL)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-lib")

add_executable(${PROJECT_NAME}-test test/main.c)
target_link_libraries(${PROJECT_NAME}-test ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME}-test PROPERTIES FOLDER "fck-tests")

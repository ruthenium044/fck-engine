include_guard(GLOBAL)

project(fck-glsl-reflection)

find_package(hash)
find_package(fckc)
find_package(kll)
find_package(api)

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}-interface INTERFACE)

add_library(fck::glsl-reflection ALIAS ${PROJECT_NAME})
add_library(fck::glsl-reflection-interface ALIAS ${PROJECT_NAME}-interface)

target_include_directories(${PROJECT_NAME}-interface INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_sources(${PROJECT_NAME}-interface PRIVATE "include/fck_glsl_reflection.h")
target_sources(${PROJECT_NAME} PRIVATE "src/fck_glsl_reflection.c")

target_link_libraries(${PROJECT_NAME}-interface INTERFACE fck::fckc)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::hash)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::api)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::kll)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::api-interface)
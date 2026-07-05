include_guard(GLOBAL)

project(fck-gfx)

find_package(fckc)
find_package(api)
find_package(kll)
find_package(os)
find_package(render)
find_package(glsl-reflection)
find_package(shader)

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}-interface INTERFACE)

add_library(fck::gfx ALIAS ${PROJECT_NAME})
add_library(fck::gfx-interface ALIAS ${PROJECT_NAME}-interface)

target_include_directories(${PROJECT_NAME}-interface INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_sources(${PROJECT_NAME}-interface PRIVATE "include/fck_gfx.h")
target_sources(${PROJECT_NAME} PRIVATE "src/fck_gfx.c")

target_link_libraries(${PROJECT_NAME}-interface INTERFACE fck::fckc)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::api)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::kll)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::os)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::shader-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::render)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::glsl-reflection-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::api-interface)

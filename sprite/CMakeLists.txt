cmake_minimum_required(VERSION 3.20)

project(fck-sprite)

include_guard(GLOBAL)

find_package(fckc)
find_package(kll)
find_package(api)
find_package(os)
find_package(render)
find_package(gfx)
find_package(png)

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}-interface INTERFACE)

add_library(fck::sprite ALIAS ${PROJECT_NAME})
add_library(fck::sprite-interface ALIAS ${PROJECT_NAME}-interface)

target_include_directories(${PROJECT_NAME}-interface INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(${PROJECT_NAME}-interface INTERFACE fck::fckc)
target_sources(${PROJECT_NAME}-interface PRIVATE "include/fck_sprite.h")

target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}-interface)

target_include_directories(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_sources(${PROJECT_NAME} PRIVATE "include/fck_sprite.h")
target_sources(${PROJECT_NAME} PRIVATE "src/fck_sprite.c")

target_link_libraries(${PROJECT_NAME} PRIVATE fck::gfx-interface)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::fckc fck::kll fck::api-interface fck::render)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::os)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::png-interface)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    fck_sprite_resource_path="${CMAKE_CURRENT_SOURCE_DIR}/resources/"
)

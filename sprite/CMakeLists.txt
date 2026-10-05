include_guard(GLOBAL)

cmake_minimum_required(VERSION 3.20)

project(fck-sprite)

find_package(fckc)
find_package(kll)
find_package(api)
find_package(os)
find_package(hash)
find_package(render)
find_package(gfx)
find_package(texture)
find_package(db)

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}-interface INTERFACE)

add_library(fck::sprite ALIAS ${PROJECT_NAME})
add_library(fck::sprite-interface ALIAS ${PROJECT_NAME}-interface)

target_include_directories(${PROJECT_NAME}-interface INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_sources(${PROJECT_NAME}-interface PRIVATE "include/fck_sprite.h")
target_sources(${PROJECT_NAME} PRIVATE "src/fck_sprite.c")

target_link_libraries(${PROJECT_NAME}-interface INTERFACE fck::fckc)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::gfx-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::kll)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::api-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::render)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::os)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::hash)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::db-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::texture-interface)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    fck_sprite_resource_path="${CMAKE_CURRENT_SOURCE_DIR}/resources/"
)

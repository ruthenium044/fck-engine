include_guard(GLOBAL)

project(fck-nuklear-texture)

find_package(api)
find_package(db)
find_package(nuklear)
find_package(texture)

add_library(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE "src/fck_nuklear_texture.c")

target_link_libraries(${PROJECT_NAME} PRIVATE fck::api-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::db-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::nuklear-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::texture-interface)

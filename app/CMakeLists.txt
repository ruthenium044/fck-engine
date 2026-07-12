# fckc
cmake_minimum_required(VERSION 3.20)

include_guard(GLOBAL)

project(fck-app)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE "main.c")


find_package(api)
find_package(os)
find_package(hash)
find_package(kll)
find_package(input)
find_package(plugins)
find_package(render)
find_package(shader)
find_package(png)
find_package(gfx)
find_package(nuklear)
find_package(sprite)
find_package(ec)
find_package(gameloop)

target_link_libraries(${PROJECT_NAME} PRIVATE fck::hash)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::os)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::kll)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::input)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::api-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::plugins-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::nuklear-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::png-interface)
# Should this be fck::render-interface? 
target_link_libraries(${PROJECT_NAME} PRIVATE fck::render)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::gameloop)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::shader-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::gfx-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::sprite-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::ec-interface)

# this needs to get substituted well...
target_compile_definitions(${PROJECT_NAME} PRIVATE
    fck_resource_path="${CMAKE_CURRENT_SOURCE_DIR}/resources/"
)

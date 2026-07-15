include_guard(GLOBAL)

project(fck-nuklear)

find_package(fckc)
find_package(kll)
find_package(render)
find_package(api)
find_package(os)
find_package(glsl-reflection)
find_package(shader)
find_package(input)
find_package(gfx)

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}-interface INTERFACE)

add_library(fck::nuklear ALIAS ${PROJECT_NAME})
add_library(fck::nuklear-interface ALIAS ${PROJECT_NAME}-interface)

target_include_directories(${PROJECT_NAME}-interface INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_sources(${PROJECT_NAME}-interface PRIVATE "include/fck_nuklear.h")
target_sources(${PROJECT_NAME}-interface PRIVATE "src/nuklear.h")
target_sources(${PROJECT_NAME}-interface PRIVATE "src/nuklear_overview_demo.inl")
target_sources(${PROJECT_NAME} PRIVATE "src/fck_nuklear.c")

target_link_libraries(${PROJECT_NAME}-interface INTERFACE fck::fckc)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::kll)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::os)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::input)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::shader-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::gfx-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::render)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::glsl-reflection-interface)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::api-interface)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    fck_nuklear_resource_path="${CMAKE_CURRENT_SOURCE_DIR}/resources/"
)

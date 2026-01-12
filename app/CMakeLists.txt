# Super Async Development
cmake_minimum_required(VERSION 3.28)

project(fck-app)

include_guard(GLOBAL)

# I should make this a command line argument
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

#if (NOT DEFINED FCK_WORKSPACE_PATH)
#    set(FCK_WORKSPACE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "Workspace Path")
#endif()

file(GLOB_RECURSE FOUND_APP_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/**.h")
file(GLOB_RECURSE FOUND_APP_C_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.c")

#add_executable(${PROJECT_NAME} ${FOUND_APP_C_SOURCES})
add_library(${PROJECT_NAME} SHARED ${FOUND_RENDER_SDL_C_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_sources(${PROJECT_NAME}
    PRIVATE ${FOUND_APP_C_SOURCES}
    PRIVATE ${FOUND_APP_HEADERS} 
)

# TODO: We should not spill this bad boy out
target_compile_definitions(${PROJECT_NAME} PRIVATE FCK_APP_ASSET_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/")

find_package(events)
find_package(fck-lib)
find_package(fckc)
find_package(ser-ext)
find_package(ser-json)
find_package(kll)
find_package(set)
find_package(dynarr)
find_package(canvas)
find_package(imgui)
find_package(render)
find_package(std)
find_package(render-vk)
find_package(shader)

find_package(app-loadable)
find_package(main)

#find_package(img)
find_package(img-sdl)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::fckc fck::hash fck::kll fck::shader fck::events fck::app-loadable fck::img fck::img-sdl fck::render fck::render-vk fck::imgui fck::canvas fck::ser-ext fck::set fck::dynarr fck::ser-json fck::std fck::ts fck::api)
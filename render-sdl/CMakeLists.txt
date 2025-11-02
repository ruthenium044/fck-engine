# KiLLer Allocators
cmake_minimum_required(VERSION 3.20)

project(fck-render-sdl)

include_guard(GLOBAL)
include(FetchContent)

FetchContent_Declare(
    SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.24
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL)

file(GLOB_RECURSE FOUND_RENDER_SDL_C_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.c")

add_library(${PROJECT_NAME} SHARED ${FOUND_RENDER_SDL_C_SOURCES})
add_library(fck::render-sdl ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE ${FOUND_RENDER_SDL_C_SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)



find_package(fckc CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
find_package(render CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
find_package(std CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
find_package(api CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::fckc fck::render fck::hash fck::set fck::kll fck::std fck::api SDL3::SDL3)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-lib")

add_executable(${PROJECT_NAME}-test test/main.c)
target_link_libraries(${PROJECT_NAME}-test ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME}-test PROPERTIES FOLDER "fck-tests")

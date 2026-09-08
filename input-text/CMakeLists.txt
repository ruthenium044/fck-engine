cmake_minimum_required(VERSION 3.20)

project(fck-input-text)

include_guard(GLOBAL)

add_library(${PROJECT_NAME})

find_package(fckc)
find_package(input)
find_package(api)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::fckc fck::input fck::api-interface)

target_sources(${PROJECT_NAME} PRIVATE "src/fck_input.sdl.text.c")

include(FetchContent)
FetchContent_Declare(
        SDL
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.24
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
        SYSTEM
)
FetchContent_MakeAvailable(SDL)

target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3)

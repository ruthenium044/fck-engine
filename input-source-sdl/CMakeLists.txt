# KiLLer Allocators
cmake_minimum_required(VERSION 3.20)

project(fck-input-source-sdl)

include_guard(GLOBAL)

FetchContent_Declare(
    SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.24
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL)


file(GLOB_RECURSE FOUND_INPUT_SOURCE_SDL_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/**.h")
file(GLOB_RECURSE FOUND_INPUT_SOURCE_SDL_C_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.c")

add_library(${PROJECT_NAME} SHARED ${FOUND_INPUT_SOURCE_SDL_C_SOURCES})
add_library(fck::input-source-sdl ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE ${FOUND_INPUT_SOURCE_SDL_C_SOURCES}
    PRIVATE ${FOUND_INPUT_SOURCE_SDL_HEADERS} 
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

find_package(fckc CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::fckc SDL3::SDL3)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-lib")

add_executable(${PROJECT_NAME}-test test/main.c)
target_link_libraries(${PROJECT_NAME}-test ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME}-test PROPERTIES FOLDER "fck-tests")

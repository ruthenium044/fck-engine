# KiLLer Allocators
cmake_minimum_required(VERSION 3.20)

project(fck-main)

include_guard(GLOBAL)

FetchContent_Declare(
    SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.24
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL)

file(GLOB_RECURSE FOUND_MAIN_C_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.c")

add_executable(${PROJECT_NAME} ${FOUND_MAIN_C_SOURCES})

target_sources(${PROJECT_NAME}
    PRIVATE ${FOUND_MAIN_C_SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

find_package(fckc CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
find_package(std CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
find_package(events CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
find_package(app-loadable CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::fckc fck::app-loadable SDL3::SDL3 fck::std fck::events)
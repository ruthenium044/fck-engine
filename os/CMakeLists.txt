cmake_minimum_required(VERSION 3.20)

project(fck-os)

include_guard(GLOBAL)

add_library(${PROJECT_NAME})

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /we4244)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Weverything
    )
endif()

add_library(fck::os ALIAS ${PROJECT_NAME})

include(FetchContent)
FetchContent_Declare(
    SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.24
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL TRUE
    SYSTEM
)
FetchContent_MakeAvailable(SDL)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

if(APPLE)
    #target_sources(${PROJECT_NAME} PRIVATE "src/fck_os.macos.c")
    # TODO: Dlete this branch - was a nice exercise, but fuck it
    # wait, why do I need these libraries - Might be leftovers, might be needed in render-vk
    target_link_libraries(${PROJECT_NAME} PRIVATE 
    "-framework CoreServices"
    "-framework AppKit"
    "-framework CoreVideo"
    "-framework Cocoa"
    "-framework CoreServices"
    "-framework QuartzCore")
endif()

target_sources(${PROJECT_NAME} PRIVATE "src/fck_os.sdl.c")
target_sources(${PROJECT_NAME} PRIVATE "include/fck_os.h")

# Currently, os is the only shared object that exports a non-load symbol, 
# so I also allow to export set symbols for convenience
# TODO: Maybe remove them one day
target_sources(${PROJECT_NAME} PRIVATE "include/fck_set.h")
target_sources(${PROJECT_NAME} PRIVATE "include/fck_set.inl")
target_sources(${PROJECT_NAME} PRIVATE "include/fck_dynarr.h")
target_sources(${PROJECT_NAME} PRIVATE "include/fck_dynarr.inl")

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

find_package(fckc)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::fckc)

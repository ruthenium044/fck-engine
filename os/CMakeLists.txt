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
    SYSTEM
)
FetchContent_MakeAvailable(SDL)

target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3)

if(APPLE)
    #target_sources(${PROJECT_NAME} PRIVATE "src/fck_os.macos.c")
    # TODO: Dlete this branch - was a nice exercise, but fuck it
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
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

find_package(fckc)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::fckc)

cmake_minimum_required(VERSION 3.20)

project(fck-os)

include_guard(GLOBAL)

add_library(${PROJECT_NAME})

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /we4244)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall 
        -Wextra 
        -Wconversion
        -Wdouble-promotion
        -Wpedantic 
        -Wnull-dereference
        -Wimplicit-int-float-conversion
        -Werror
        -Wformat=2
        -Wshadow
        -Wpointer-arith
        -Wcast-align
        -Wwrite-strings
        -Wmissing-prototypes
        -Wstrict-prototypes
        -Wold-style-definition
    )
endif()

add_library(fck::os ALIAS ${PROJECT_NAME})

if(APPLE)
    target_sources(${PROJECT_NAME} PRIVATE "src/fck_os.macos.c")
    # TODO: Dlete this branch - was a nice exercise, but fuck it
    find_package(kll)
    target_link_libraries(${PROJECT_NAME} PUBLIC fck::fckc fck::events fck::kll
    "-framework AppKit"
    "-framework CoreVideo"
    "-framework Cocoa"     
    "-framework QuartzCore")
else()
    target_sources(${PROJECT_NAME} PRIVATE "src/fck_os.sdl.c")
    #if(BUILD_SHARED_LIBS)
    #    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    #    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    #else()
    #    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    #    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    #endif()

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
endif()

target_sources(${PROJECT_NAME} PRIVATE "include/fck_os.h")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

find_package(fckc)

target_link_libraries(${PROJECT_NAME} PRIVATE fck::fckc)

add_executable(${PROJECT_NAME}-test "test/main.c")
target_link_libraries(${PROJECT_NAME}-test PRIVATE ${PROJECT_NAME} PRIVATE fck::fckc)

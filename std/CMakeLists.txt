cmake_minimum_required(VERSION 3.20)

project(fck-std)

include_guard(GLOBAL)

add_library(${PROJECT_NAME})
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall 
        -Wextra 
        -Wpedantic 
        -Werror
        -Wshadow
        -Wpointer-arith
        -Wcast-align
        -Wwrite-strings
        -Wmissing-prototypes
        -Wstrict-prototypes
    )
endif()

add_library(fck::std ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE "src/fck_input.c")

if(APPLE)
    target_sources(${PROJECT_NAME} PRIVATE "src/fck_os.macos.c")

    find_package(kll)
    target_link_libraries(${PROJECT_NAME} PUBLIC fck::fckc fck::events fck::kll
    "-framework AppKit"
    "-framework CoreVideo"
    "-framework Cocoa"     
    "-framework QuartzCore")
else()
    target_sources(${PROJECT_NAME} PRIVATE "src/fck_os.sdl.c")
    target_sources(${PROJECT_NAME} PRIVATE "src/fck_input.sdl.mouse.c")
    target_sources(${PROJECT_NAME} PRIVATE "src/fck_input.sdl.text.c")
    target_sources(${PROJECT_NAME} PRIVATE "src/fck_input.sdl.physical_keyboard.c")

    if(BUILD_SHARED_LIBS)
        set(SDL_SHARED ON CACHE BOOL "" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    else()
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
    endif()

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
target_sources(${PROJECT_NAME} PRIVATE "include/fck_input.h")
target_sources(${PROJECT_NAME} PRIVATE "include/fck_mouse.h")
target_sources(${PROJECT_NAME} PRIVATE "include/fck_pkey.h")
target_sources(${PROJECT_NAME} PRIVATE "include/fck_text_input.h")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

find_package(events)
find_package(fckc)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::fckc PRIVATE fck::events)

add_executable(${PROJECT_NAME}-test "test/main.c")
target_link_libraries(${PROJECT_NAME}-test PRIVATE ${PROJECT_NAME} fck::fckc fck::events)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-lib")
set_target_properties(${PROJECT_NAME}-test PROPERTIES FOLDER "fck-tests")
cmake_minimum_required(VERSION 3.20)
project(fck)

# Verbosity settings
set(FETCHCONTENT_QUIET TRUE)
set(CMAKE_MESSAGE_LOG_LEVEL WARNING)

include(FetchContent)
include(ExternalProject)

# CMake settings
set(CMAKE_CXX_STANDARD 17)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# SDL settings
set(BUILD_SHARED_LIBS OFF)
set(SDLIMAGE_AVIF OFF)
set(SDL_SHARED FALSE)
set(SDL_STATIC TRUE)

# Sets the library file ending so we can build for windows, apple and others
if(APPLE)
    set(PLATFORM_LIBRARY_EXTENSION ".a")
    enable_language(OBJC)
elseif(UNIX)
    set(PLATFORM_LIBRARY_EXTENSION ".a")
else()
    set(PLATFORM_LIBRARY_EXTENSION ".lib")
endif()

set(SDL_TESTS OFF)
set(SDL_TEST OFF)

# Move this to git outside of cmake
# SDL
FetchContent_Declare(
    SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG "main"
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

# Fetch SDL3_image
FetchContent_Declare(
        SDL_image
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
        GIT_TAG "main"
)

# Fetch SDL3_net
FetchContent_Declare(
        SDL_net
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_net.git
        GIT_TAG "main"
)

# For each SDL module, we should set on a GIT_TAG so we do not have funny renames and other issues!
FetchContent_MakeAvailable(SDL SDL_image SDL_net)

# LOCAL PROJECT
file(GLOB_RECURSE FOUND_CXX_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/**.h")
list(APPEND HEADERS ${FOUND_CXX_HEADERS})

file(GLOB_RECURSE FOUND_CXX_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp")
list(APPEND SOURCES ${FOUND_CXX_SOURCES})

# We need to check out if this is needed 
#include_directories(fck "include")

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Shouldn't be needed? We need to check it out
#target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
#source_group("Header Files" FILES ${FOUND_CXX_HEADERS})

add_dependencies(fck SDL3::SDL3)

target_link_libraries(${PROJECT_NAME} SDL3::SDL3 SDL3_image::SDL3_image SDL3_net::SDL3_net)
cmake_minimum_required(VERSION 3.20)
project(fck)

# Verbosity settings
#set(FETCHCONTENT_QUIET TRUE)
#set(CMAKE_MESSAGE_LOG_LEVEL WARNING)

# Optionally, you can set SDL-specific CMake options here

include(FetchContent)
include(ExternalProject)

set(CMAKE_CXX_STANDARD 23)

# Sets the library file ending so we can build for windows, apple and others
if(APPLE)
    set(PLATFORM_LIBRARY_EXTENSION ".a")
    if(APPLE)
        enable_language(OBJC)
    endif()
elseif(UNIX)
    set(PLATFORM_LIBRARY_EXTENSION ".a")
else()
    set(PLATFORM_LIBRARY_EXTENSION ".lib")
endif()

set(SDL_TEST OFF)
set(SDL_SHARED TRUE CACHE BOOL "Build a SDL shared library (if available)")
set(SDL_STATIC TRUE CACHE BOOL "Build a SDL static library (if available)")

# Move this to git outside of cmake
# SDL
FetchContent_Declare(
    SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG "prerelease-3.0.0"
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(SDL)
list(APPEND CMAKE_PREFIX_PATH SDL3)

#find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)
# !SDL

# SDL-image
#FetchContent_Declare(
#    SDL_image
#    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
#    GIT_TAG "main" #"release-2.8.2"
#    GIT_SHALLOW TRUE
#    GIT_PROGRESS TRUE
#)

#set(SDLIMAGE_INSTALL OFF)

#FetchContent_MakeAvailable(SDL_image)
#add_dependencies(SDL_image SDL)
# !SDL-image

# LUA
# Download, but not built
FetchContent_Declare(
    lua
    URL https://www.lua.org/ftp/lua-5.4.6.tar.gz  # URL to Lua's source tarball
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(lua)

# Build in source directly since we need to build with make
set(LUA_LIBRARY_FILE_PATH "${lua_SOURCE_DIR}/src/liblua${PLATFORM_LIBRARY_EXTENSION}")
ExternalProject_Add(
    lua 
    SOURCE_DIR ${lua_SOURCE_DIR}
    CONFIGURE_COMMAND ""
    BUILD_ALWAYS TRUE
    BUILD_IN_SOURCE TRUE
    BUILD_COMMAND make all test
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS ${LUA_LIBRARY_FILE_PATH} # Byproduct for ninja so it knows what to expect
)
add_library(liblua IMPORTED STATIC GLOBAL)

# Wait for lua make to be done and setup liblua 
add_dependencies(liblua lua)

ExternalProject_Get_property(lua BINARY_DIR)
ExternalProject_Get_property(lua SOURCE_DIR)
set_target_properties(liblua PROPERTIES
    IMPORTED_LOCATION ${LUA_LIBRARY_FILE_PATH} # Ninja knows it will have this path available as byproduct
    INTERFACE_INCLUDE_DIRECTORIES ${lua_SOURCE_DIR}/src # Re-use downloaded fetch source since it is guaranteed
)
# !LUA

# LOCAL PROJECT
file(GLOB_RECURSE FOUND_CXX_SOURCES "src/**.cpp")
list(APPEND SOURCES ${FOUND_CXX_SOURCES})

include_directories(fck "include")

add_executable(${PROJECT_NAME} ${SOURCES})

add_dependencies(fck liblua SDL3::SDL3)

target_link_libraries(${PROJECT_NAME} SDL3::SDL3 liblua)
cmake_minimum_required(VERSION 3.28)
project(fck)

include(FetchContent)
include(ExternalProject)

set(CMAKE_CXX_STANDARD 23)

# Sets the library file ending so we can build for windows, apple and others
if(NOT APPLE)
    set(PLATFORM_LIBRARY_EXTENSION ".lib")
else()
    set(PLATFORM_LIBRARY_EXTENSION ".a")
    if(APPLE)
        enable_language(OBJC)
    endif()
endif()

Set(FETCHCONTENT_QUIET TRUE)

# Move this to git outside of cmake
# SDL
FetchContent_Declare(
    SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG d6ad28a4cb87b8bb00893f049f7e6e72063fc3b6
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(SDL)
# !SDL

# LUA
# Download, but not built
FetchContent_Declare(
    lua
    URL https://www.lua.org/ftp/lua-5.4.6.tar.gz  # URL to Lua's source tarball
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

add_dependencies(fck liblua)

target_link_libraries(${PROJECT_NAME} SDL3::SDL3 liblua)

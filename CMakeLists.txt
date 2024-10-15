cmake_minimum_required(VERSION 3.20)
project(fck)

# Verbosity settings
#set(FETCHCONTENT_QUIET TRUE)
#set(CMAKE_MESSAGE_LOG_LEVEL ERROR)

include(FetchContent)

# CMake settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
#set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# We fix this up later to be optimised for release and non-optimised for debug
if(MSVC)
    set(CMAKE_CXX_FLAGS "-Od")
    add_compile_options(/wd4003)  # Disable specific warning code 4003
endif()

# SDL settings
set(SDLIMAGE_AVIF OFF)
set(SDLIMAGE_BMP OFF)
set(SDLIMAGE_GIF OFF)
set(SDLIMAGE_JPG OFF)
set(SDLIMAGE_JXL OFF)
set(SDLIMAGE_LBM OFF)
set(SDLIMAGE_PCX OFF)
set(SDLIMAGE_PNM OFF)
set(SDLIMAGE_QOI OFF)
set(SDLIMAGE_SVG OFF)
set(SDLIMAGE_TGA OFF)
set(SDLIMAGE_TIF OFF)
set(SDLIMAGE_WEBP OFF)
set(SDLIMAGE_XCF OFF)
set(SDLIMAGE_XPM OFF)
set(SDLIMAGE_XV OFF)
set(SDLIMAGE_PNG ON)

set(BUILD_SHARED_LIBS OFF)
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
    GIT_TAG 4f722d372ae7246f123762b0407cbec1e6e71d65
    GIT_PROGRESS TRUE
)

# Fetch SDL3_image
FetchContent_Declare(
    SDL_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG c1b272450d306db3845086cc231acae736b92f4f
    GIT_PROGRESS TRUE
)

# Fetch SDL3_net
FetchContent_Declare(
    SDL_net
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_net.git
    GIT_TAG 7ffe77830cad41c7edbacc8eda35cf99a7557072
    GIT_PROGRESS TRUE
)

# For each SDL module, we should set on a GIT_TAG so we do not have funny renames and other issues!
FetchContent_MakeAvailable(SDL SDL_image SDL_net)

find_package(Vulkan REQUIRED)


# LOCAL PROJECT
file(GLOB_RECURSE FOUND_CXX_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/**.h")
list(APPEND HEADERS ${FOUND_CXX_HEADERS})

file(GLOB_RECURSE FOUND_CXX_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp")
list(APPEND SOURCES ${FOUND_CXX_SOURCES})

# We need to check out if this is needed 
include_directories(fck include)

# Idk if this is needed
source_group("Header Files" FILES ${FOUND_CXX_HEADERS})

# Resource path
set(FCK_RESOURCE_DIRECTORY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/")

# We can also use file commands for a release non-engine build 
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Maybe later
target_compile_definitions(${PROJECT_NAME} PRIVATE "FCK_RESOURCE_DIRECTORY_PATH= \"${FCK_RESOURCE_DIRECTORY_PATH}\"")

list(APPEND LIBRARIES SDL3::SDL3 SDL3_image::SDL3_image SDL3_net::SDL3_net)
list(APPEND LIBRARIES Vulkan::Vulkan)

add_dependencies(fck ${LIBRARIES})

target_link_libraries(${PROJECT_NAME} ${LIBRARIES})
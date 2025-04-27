cmake_minimum_required(VERSION 3.20)
project(fck)

# Verbosity settings
message("Setting up fck-engine...")

if(APPLE)
    enable_language(OBJC)
endif()
#message("Setup might be silent - Downloading dependencies and configuring cmake project")
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
    add_compile_options(/wd4003 /arch:AVX2)  # Disable specific warning code 4003
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

# After checking the SDL CMakeLists, I keep this off
# Might cause overhead, we will see what else that cascades into
set(SDL_TESTS OFF)
set(SDL_TEST OFF)

# Move this to git outside of cmake
# SDL
FetchContent_Declare(
    SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.0
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)

# Fetch SDL3_image
FetchContent_Declare(
    SDL_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG prerelease-3.1.1
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)

# SDL TTF
FetchContent_Declare(
    SDL_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf
    GIT_TAG preview-3.1.0
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)

# For each SDL module, we should set on a GIT_TAG so we do not have funny renames and other issues!
FetchContent_MakeAvailable(SDL SDL_image SDL_ttf)

# Maybe disable Vulkan conditionally - is it even needed with SDL_GPU at this point?
find_package(Vulkan REQUIRED)

if(DEFINED FCK_LIBRARY_MODE AND NOT DEFINED FCK_RESOURCE_DIRECTORY_PATH) 
	message(FATAL_ERROR "GEN_INPUT_DIRECTORY_PATH is not defined. Needs to define a path for output. (For example: set(FCK_RESOURCE_DIRECTORY_PATH [path]))")
endif()

# LOCAL PROJECT
file(GLOB_RECURSE FOUND_CXX_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/**.h")
list(APPEND HEADERS ${FOUND_CXX_HEADERS})

file(GLOB_RECURSE FOUND_CXX_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp")
list(APPEND SOURCES ${FOUND_CXX_SOURCES})

file(GLOB_RECURSE FOUND_C_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.c")
list(APPEND SOURCES ${FOUND_C_SOURCES})

# Define resource path
if(NOT DEFINED FCK_LIBRARY_MODE) 
    set(FCK_RESOURCE_DIRECTORY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources")
endif()

set(GEN_OUTPUT_DIRECTORY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(GEN_INPUT_DIRECTORY_PATH ${FCK_RESOURCE_DIRECTORY_PATH})
set(GEN_SOURCE_DIRECTORY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include")
# Generator

add_subdirectory(gen)

add_custom_target(gen_output
     COMMAND ${CMAKE_CURRENT_BINARY_DIR}/gen/gen
     DEPENDS gen)

include_directories(${PROJECT_NAME} include)

# Idk if this is needed
source_group("Header Files" FILES ${FOUND_CXX_HEADERS})

# target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

# We can also use file commands for a release non-engine build 
# TODO: Revise library mode and its implementation later!
if(FCK_LIBRARY_MODE)
    list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main/main.cpp")

    set(fck_target_name fck)
    add_library(${fck_target_name} ${SOURCES})
    add_library(fck::${fck_target_name} ALIAS ${fck_target_name})

    target_include_directories(${fck_target_name}
        PUBLIC
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )
    install(
        TARGETS ${fck_target_name}
        EXPORT fckExports
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT devel
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT library
    )
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
endif()

# Generator
add_dependencies(${PROJECT_NAME} gen_output)
# !Generator

# Maybe later
target_compile_definitions(${PROJECT_NAME} PRIVATE "FCK_RESOURCE_DIRECTORY_PATH= \"${FCK_RESOURCE_DIRECTORY_PATH}\"")

# Warning levels
#target_compile_options(${PROJECT_NAME} PRIVATE
#  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
#  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
#)

list(APPEND LIBRARIES SDL3::SDL3 SDL3_image::SDL3_image SDL3_ttf::SDL3_ttf)
list(APPEND LIBRARIES Vulkan::Vulkan)

add_dependencies(${PROJECT_NAME} ${LIBRARIES})

if(WIN32)
    list(APPEND LIBRARIES iphlpapi ws2_32)
endif()
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

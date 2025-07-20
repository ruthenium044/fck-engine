# Super Async Development
cmake_minimum_required(VERSION 3.20)
include(FetchContent)

project(fck-app)

include_guard(GLOBAL)

if (NOT DEFINED FCK_WORKSPACE_PATH)
    set(FCK_WORKSPACE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "Workspace Path")
endif()
find_package(fck-lib CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH})

set(BUILD_SHARED_LIBS OFF)
set(SDL_SHARED FALSE)
set(SDL_STATIC TRUE)
FetchContent_Declare(
    SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.0
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL)

file(GLOB_RECURSE FOUND_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/**.h")
list(APPEND HEADERS ${FOUND_HEADERS})

file(GLOB_RECURSE FOUND_INL_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/**.inl")
list(APPEND HEADERS ${FOUND_HEADERS})

file(GLOB_RECURSE FOUND_C_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.c")

include_directories(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable(${PROJECT_NAME} ${SOURCES})

target_sources(${PROJECT_NAME}
    PRIVATE ${FOUND_C_SOURCES}
    PRIVATE ${FOUND_INL_HEADERS}
    PRIVATE ${FOUND_HEADERS} 
)

find_package(fckc CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::fckc fck::kll PRIVATE SDL3::SDL3)

set_target_properties(SDL3-static PROPERTIES FOLDER "SDL3")
set_target_properties(SDL_uclibc PROPERTIES FOLDER "SDL3")
set_target_properties(SDL3_test PROPERTIES FOLDER "SDL3")

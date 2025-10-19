# Super Async Development
cmake_minimum_required(VERSION 3.20)
include(FetchContent)

project(fck-app)

include_guard(GLOBAL)

if (NOT DEFINED FCK_WORKSPACE_PATH)
    set(FCK_WORKSPACE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "Workspace Path")
endif()
find_package(fck-lib CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH})

FetchContent_Declare(
    SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.0
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL)

file(GLOB_RECURSE FOUND_APP_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/**.h")
file(GLOB_RECURSE FOUND_APP_C_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.c")

include_directories(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable(${PROJECT_NAME} ${FOUND_APP_C_SOURCES})

target_sources(${PROJECT_NAME}
    PRIVATE ${FOUND_APP_C_SOURCES}
    PRIVATE ${FOUND_APP_HEADERS} 
)

find_package(fckc CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
find_package(ser-ext CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
find_package(ser-json CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)

# Fix up truely shared libraries!
#target_link_options(${PROJECT_NAME} PRIVATE LINKER:-no_warn_duplicate_libraries)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::fckc fck::hash PRIVATE fck::ser-ext fck::ser-json fck::std fck::kll fck::ts fck::api SDL3::SDL3)

#set_target_properties(SDL3 PROPERTIES FOLDER "SDL3")
set_target_properties(SDL_uclibc PROPERTIES FOLDER "SDL3")
set_target_properties(SDL3_test PROPERTIES FOLDER "SDL3")
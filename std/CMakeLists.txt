cmake_minimum_required(VERSION 3.20)

project(fck-std)

include_guard(GLOBAL)
include(FetchContent)

FetchContent_Declare(
    SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.24
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL)

add_library(${PROJECT_NAME} SHARED "src/fck_os.c")
add_library(fck::std ALIAS ${PROJECT_NAME})

# because... mac? 
# set_target_properties(${PROJECT_NAME}  PROPERTIES
#   FRAMEWORK TRUE
#   FRAMEWORK_VERSION A
#   MACOSX_FRAMEWORK_IDENTIFIER org.cmake.fck.os
# )

target_sources(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/fck_os.h
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/fck_os.c
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-exports
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

find_package(fckc CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::fckc PRIVATE SDL3::SDL3)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-lib")

add_executable(${PROJECT_NAME}-test test/main.c)
target_link_libraries(${PROJECT_NAME}-test INTERFACE ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME}-test PROPERTIES FOLDER "fck-tests")

set_target_properties(SDL3-shared PROPERTIES FOLDER "SDL3")
set_target_properties(SDL_uclibc PROPERTIES FOLDER "SDL3")
set_target_properties(SDL3_test PROPERTIES FOLDER "SDL3")

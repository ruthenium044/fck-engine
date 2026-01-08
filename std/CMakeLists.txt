cmake_minimum_required(VERSION 3.20)

project(fck-std)

include_guard(GLOBAL)

find_package(events)
find_package(fckc)

if(APPLE)
    find_package(kll)
    add_library(${PROJECT_NAME} SHARED)
    target_sources(${PROJECT_NAME}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/fck_os.h
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/fck_os.macos.c
    )

    target_link_libraries(${PROJECT_NAME} PUBLIC fck::fckc fck::events fck::kll
    "-framework AppKit"
    "-framework CoreVideo"
    "-framework Cocoa"     
    "-framework QuartzCore")
else()
    include(FetchContent)
    FetchContent_Declare(
        SDL
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.24
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(SDL)

    add_library(${PROJECT_NAME} SHARED)
        target_sources(${PROJECT_NAME}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/fck_os.h
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/fck_os.sdl.c
    )

    target_link_libraries(${PROJECT_NAME} PUBLIC fck::fckc fck::events SDL3::SDL3)
endif()
    
add_library(fck::std ALIAS ${PROJECT_NAME})


target_include_directories(${PROJECT_NAME}
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-lib")

add_executable(${PROJECT_NAME}-test test/main.c)
target_link_libraries(${PROJECT_NAME}-test PUBLIC ${PROJECT_NAME} fck::fckc fck::events)
set_target_properties(${PROJECT_NAME}-test PROPERTIES FOLDER "fck-tests")
# KiLLer Allocators
cmake_minimum_required(VERSION 3.24)

project(fck-render-vk)

include_guard(GLOBAL)
include(FetchContent)

file(GLOB_RECURSE FOUND_RENDER_VK_C_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.c")

add_library(${PROJECT_NAME} SHARED ${FOUND_RENDER_VK_C_SOURCES})
add_library(fck::render-vk ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE ${FOUND_RENDER_VK_C_SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

find_package(fckc CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
find_package(render CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
find_package(std CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
find_package(api CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
find_package(events CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
find_package(kll CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)

#find_package(Vulkan)
find_package(Vulkan REQUIRED)

message(${Vulkan_LIBRARY})
#target_link_libraries(${PROJECT_NAME} PRIVATE fck::fckc fck::render fck::hash fck::set fck::kll fck::std fck::api SDL3::SDL3)
#set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-lib")

add_executable(${PROJECT_NAME}-test test/main.c)
target_link_libraries(${PROJECT_NAME}-test PRIVATE fck::std fck::kll fck::events Vulkan::Vulkan
    "-framework AppKit"
    "-framework CoreVideo"
    "-framework Cocoa"     
    "-framework QuartzCore"
    "-framework Metal"
    "-framework MetalKit"  
    )
set_target_properties(${PROJECT_NAME}-test PROPERTIES FOLDER "fck-tests")

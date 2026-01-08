# KiLLer Allocators
cmake_minimum_required(VERSION 3.24)

project(fck-render-vk)

include_guard(GLOBAL)

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

find_package(fckc)
find_package(render)
find_package(std)
find_package(api)
find_package(hash)
find_package(events)
find_package(kll)
find_package(set)
find_package(shader)
find_package(Vulkan)

message(${Vulkan_LIBRARY})
#target_link_libraries(${PROJECT_NAME} PRIVATE fck::fckc fck::render fck::hash fck::set fck::kll fck::std fck::api SDL3::SDL3)
#set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-lib")

add_executable(${PROJECT_NAME}-test test/main.c test/sht_loader.c test/sht_loader.macos.c test/sht_memory.c)

target_sources(${PROJECT_NAME}-test
    PRIVATE test/sht_loader.h test/public.h test/sht_vk.internal.h
)

target_link_libraries(${PROJECT_NAME}-test PRIVATE fck::std fck::kll fck::set fck::events Vulkan::Vulkan fck::shader fck::hash
    "-framework AppKit"
    "-framework CoreVideo"
    "-framework Cocoa"     
    "-framework QuartzCore"
    "-framework Metal"
    "-framework MetalKit"  
    )
set_target_properties(${PROJECT_NAME}-test PROPERTIES FOLDER "fck-tests")

# KiLLer Allocators
cmake_minimum_required(VERSION 3.24)

project(fck-render-vk)

include_guard(GLOBAL)

#file(GLOB_RECURSE FOUND_RENDER_VK_C_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.c")
file(GLOB_RECURSE FOUND_HEADER_VK_C_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.h")

add_library(${PROJECT_NAME} SHARED src/sht_render.vk.c)
add_library(fck::render-vk ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE src/sht_render.vk.c
    PRIVATE src/sht_vk.platform.c
    PRIVATE ${FOUND_HEADER_VK_C_SOURCES}
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
find_package(shader)
find_package(Vulkan)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE fck::api fck::fckc fck::std fck::kll fck::render fck::events Vulkan::Vulkan fck::shader fck::hash
        "-framework Metal"
        "-framework MetalKit")
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE fck::api fck::fckc fck::std fck::kll fck::render fck::events Vulkan::Vulkan fck::shader fck::hash)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-lib")

add_executable(${PROJECT_NAME}-test test/main.c)

target_link_libraries(${PROJECT_NAME}-test PRIVATE ${PROJECT_NAME} fck::fckc fck::std fck::kll fck::render fck::events fck::shader)
set_target_properties(${PROJECT_NAME}-test PROPERTIES FOLDER "fck-tests")

# KiLLer Allocators
cmake_minimum_required(VERSION 3.24)

project(fck-render-vk)

include_guard(GLOBAL)

add_library(${PROJECT_NAME})
add_library(fck::render-vk ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE "src/sht_vk.internal.h")
target_sources(${PROJECT_NAME} PRIVATE "src/sht_render.vk.c")
target_sources(${PROJECT_NAME} PRIVATE "src/sht_vk.platform.c")

#target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

find_package(fckc)
find_package(render)
find_package(os)
find_package(api)
find_package(hash)
find_package(kll)
find_package(shader)
find_package(Vulkan)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PUBLIC fck::render PRIVATE fck::api fck::fckc fck::os fck::kll Vulkan::Vulkan fck::shader fck::hash
        "-framework Metal"
        "-framework MetalKit")
else()
    target_link_libraries(${PROJECT_NAME} PUBLIC fck::render PRIVATE fck::api fck::fckc fck::os fck::kll Vulkan::Vulkan fck::shader fck::hash)
endif()


add_executable(${PROJECT_NAME}-test test/main.c)
target_link_libraries(${PROJECT_NAME}-test PRIVATE ${PROJECT_NAME} fck::os fck::kll fck::render fck::shader)

set_target_properties(${PROJECT_NAME}-test PROPERTIES
    VS_DEBUGGING_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-lib")
set_target_properties(${PROJECT_NAME}-test PROPERTIES FOLDER "fck-tests")

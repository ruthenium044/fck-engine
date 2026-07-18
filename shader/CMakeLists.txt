# KiLLer Allocators
cmake_minimum_required(VERSION 3.20)

project(fck-shader)

include_guard(GLOBAL)

#set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
#set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
#set(SHADERC_ENABLE_SHARED_CRT OFF CACHE BOOL "" FORCE)
#set(SHADERC_ENABLE_SPIRV_TOOLS ON)
#set(SHADERC_ENABLE_GLSLANG ON)

find_package(Vulkan COMPONENTS REQUIRED dxc shaderc_combined)

find_package(fckc)
find_package(os)
find_package(kll)
find_package(api)

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}-interface INTERFACE)

add_library(fck::shader ALIAS ${PROJECT_NAME})
add_library(fck::shader-interface ALIAS ${PROJECT_NAME}-interface)

target_include_directories(${PROJECT_NAME}-interface INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_sources(${PROJECT_NAME}-interface PRIVATE "include/fck_shader.h")

#get_filename_component(VULKAN_LIB_DIR "${Vulkan_LIBRARY}" DIRECTORY)
    
# dxc requires some higher cxx std... no clue, do not care
#target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

#set_source_files_properties("src/fck_shader.cpp" PROPERTIES LANGUAGE CXX)
target_sources(${PROJECT_NAME} PRIVATE "src/fck_shader.cpp")


target_link_libraries(${PROJECT_NAME}-interface INTERFACE fck::fckc)

target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}-interface)

target_link_libraries(${PROJECT_NAME} PRIVATE fck::kll fck::api-interface fck::os Vulkan::Vulkan Vulkan::shaderc_combined Vulkan::dxc_lib)

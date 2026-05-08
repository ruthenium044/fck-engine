# KiLLer Allocators
cmake_minimum_required(VERSION 3.20)

project(fck-shader)

include_guard(GLOBAL)

#set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
#set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
#set(SHADERC_ENABLE_SHARED_CRT OFF CACHE BOOL "" FORCE)
#set(SHADERC_ENABLE_SPIRV_TOOLS ON)
#set(SHADERC_ENABLE_GLSLANG ON)

include(FetchContent)

add_library(${PROJECT_NAME})
add_library(fck::shader ALIAS ${PROJECT_NAME})

# dxc requires some higher cxx std... no clue, do not care
#target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

set_source_files_properties("src/fck_shader.cpp" PROPERTIES LANGUAGE CXX)

target_sources(${PROJECT_NAME} PRIVATE "include/fck_shader.h")
target_sources(${PROJECT_NAME} PRIVATE "src/fck_shader.cpp")

find_package(Vulkan COMPONENTS dxc shaderc_combined)

find_package(fckc)
find_package(os)
find_package(kll)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::fckc PRIVATE fck::kll fck::os Vulkan::shaderc_combined Vulkan::dxc_lib)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-lib")


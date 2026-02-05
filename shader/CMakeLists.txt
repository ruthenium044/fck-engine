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

file(GLOB_RECURSE FOUND_SHADER_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/**.h")
file(GLOB_RECURSE FOUND_SHADER_C_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp")

set_source_files_properties(${FOUND_SHADER_C_SOURCES} PROPERTIES LANGUAGE CXX)

add_library(${PROJECT_NAME} SHARED ${FOUND_SHADER_C_SOURCES})
add_library(fck::shader ALIAS ${PROJECT_NAME})

# dxc requires some higher cxx std... no clue, do not care
#target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_sources(${PROJECT_NAME}
    PRIVATE ${FOUND_SHADER_C_SOURCES}
    PRIVATE ${FOUND_SHADER_HEADERS} 
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

find_package(Vulkan COMPONENTS dxc shaderc_combined)

find_package(fckc)
find_package(std)
find_package(kll)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::fckc fck::kll fck::std Vulkan::shaderc_combined Vulkan::dxc_lib)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-lib")

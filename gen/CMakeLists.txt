cmake_minimum_required(VERSION 3.20)
project(gen)

set (CMAKE_CXX_STANDARD 20)

if(NOT DEFINED GEN_INPUT_DIRECTORY_PATH) 
	message(FATAL_ERROR "GEN_INPUT_DIRECTORY_PATH is not defined. Needs to define a path for output. (For example: set(GEN_RESOURCE_DIRECTORY_PATH [path]))")
endif()

if(NOT DEFINED GEN_OUTPUT_DIRECTORY_PATH) 
	message(FATAL_ERROR "GEN_OUTPUT_DIRECTORY_PATH is not defined. Needs to define a path for output. (For example: set(GEN_RESOURCE_DIRECTORY_PATH [path]))")
endif()

# LOCAL PROJECT
file(GLOB_RECURSE FOUND_CXX_GEN_HEADERS "${CMAKE_CURRENT_LIST_DIR}/**.h")

file(GLOB_RECURSE FOUND_CXX_GEN_SOURCES "${CMAKE_CURRENT_LIST_DIR}/**.cpp")

source_group("Header Files" FILES ${FOUND_CXX_GEN_HEADERS})

make_directory("${GEN_OUTPUT_DIRECTORY_PATH}/gen")

add_executable(${PROJECT_NAME} ${FOUND_CXX_GEN_SOURCES} ${FOUND_CXX_GEN_HEADERS})

target_compile_definitions(${PROJECT_NAME} PRIVATE "GEN_INPUT_DIRECTORY_PATH= \"${GEN_INPUT_DIRECTORY_PATH}\"")
target_compile_definitions(${PROJECT_NAME} PRIVATE "GEN_OUTPUT_DIRECTORY_PATH= \"${GEN_OUTPUT_DIRECTORY_PATH}\"")

# Reference for libary. In higher cmake just add_subdirectory("gen")
#set(target_name ${PROJECT_NAME})
#
#add_library(${target_name} ${GEN_SOURCES} ${GEN_HEADERS})
#add_library(${PROJECT_NAME}::${target_name} ALIAS ${target_name})
#
#target_include_directories(${target_name}
#    PUBLIC
#        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
#        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
#)
#install(
#    TARGETS ${target_name}
#    EXPORT genExports
#    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT devel
#    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
#    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT library
#)

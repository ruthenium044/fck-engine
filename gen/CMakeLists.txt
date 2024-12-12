cmake_minimum_required(VERSION 3.20)
project(gen)

# LOCAL PROJECT
file(GLOB_RECURSE FOUND_CXX_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/**.h")
list(APPEND HEADERS ${FOUND_CXX_HEADERS})

file(GLOB_RECURSE FOUND_CXX_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp")
list(APPEND SOURCES ${FOUND_CXX_SOURCES})

set(target_name ${PROJECT_NAME})

add_library(${target_name} ${SOURCES} ${HEADERS})
add_library(${PROJECT_NAME}::${target_name} ALIAS ${target_name})

target_include_directories(${target_name}
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
install(
    TARGETS ${target_name}
    EXPORT genExports
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT devel
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT library
)

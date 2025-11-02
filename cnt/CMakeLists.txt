# fck-cnt
cmake_minimum_required(VERSION 3.20)

project(fck-cnt)

include_guard(GLOBAL)

file(GLOB_RECURSE FOUND_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/**.h")
list(APPEND HEADERS ${FOUND_HEADERS})

file(GLOB_RECURSE FOUND_C_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.c")

file(GLOB_RECURSE FOUND_SOURCE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/**.h")
list(APPEND HEADERS ${FOUND_HEADERS})

list(APPEND SOURCES ${FOUND_C_SOURCES})
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c")

add_library(${PROJECT_NAME} ${SOURCES})
add_library(fck::cnt ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE ${FOUND_C_SOURCES}
    PRIVATE ${FOUND_HEADERS} ${FOUND_SOURCE_HEADERS}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)



find_package(fckc CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
target_link_libraries(${PROJECT_NAME} fck::fckc)

add_executable(${PROJECT_NAME}-test test/main.c)
target_link_libraries(${PROJECT_NAME}-test ${PROJECT_NAME})

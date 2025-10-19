cmake_minimum_required(VERSION 3.20)

project(fck-api)

include_guard(GLOBAL)

add_library(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/fck_apis.c)
add_library(fck::api ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/fck_apis.h
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/fck_apis.c
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-exports
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

find_package(hash CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
find_package(fckc CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
find_package(kll CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
find_package(std CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH} ..)
target_link_libraries(${PROJECT_NAME} PRIVATE fck::fckc fck::kll fck::hash fck::std)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-lib")

add_executable(${PROJECT_NAME}-test test/main.c)
target_link_libraries(${PROJECT_NAME}-test ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME}-test PROPERTIES FOLDER "fck-tests")

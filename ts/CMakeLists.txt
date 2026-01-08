cmake_minimum_required(VERSION 3.20)

project(fck-ts)

include_guard(GLOBAL)

file(GLOB_RECURSE FOUND_TS_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/**.h")
file(GLOB_RECURSE FOUND_TS_INL_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/**.inl")
file(GLOB_RECURSE FOUND_TS_C_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.c")

add_library(${PROJECT_NAME} SHARED ${FOUND_TS_C_SOURCES})
add_library(fck::ts ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE ${FOUND_TS_HEADERS}
    PRIVATE ${FOUND_TS_INL_HEADERS} 
    PRIVATE ${FOUND_TS_C_SOURCES} 
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

find_package(hash)
find_package(std)
find_package(fckc)
find_package(kll)
find_package(api)
find_package(ser)
find_package(set)
find_package(dynarr)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::fckc fck::ser PRIVATE fck::kll fck::set fck::dynarr fck::std fck::api fck::hash)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "fck-lib")

add_executable(${PROJECT_NAME}-test test/main.c)
target_link_libraries(${PROJECT_NAME}-test PUBLIC ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME}-test PROPERTIES FOLDER "fck-tests")

cmake_minimum_required(VERSION 3.20)

project(fck-ts)

include_guard(GLOBAL)

add_library(${PROJECT_NAME} SHARED ${FOUND_TS_C_SOURCES})
add_library(fck::ts ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/fck_type_system.h")

target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/fck_type_system.inl")
target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/fck_type_system.c")
target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/fck_type_system_identifiers.c")
target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/fck_type_system_marshal.c")
target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/fck_type_system_members.c")
target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/fck_type_system_runtime_implementation.c")
target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/fck_type_system_serialisation.c")

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

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

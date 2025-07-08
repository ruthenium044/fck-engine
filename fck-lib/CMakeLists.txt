cmake_minimum_required(VERSION 3.20)
# fck-library
project(fck-lib)

set(FCK_WORKSPACE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "Workspace Path")
find_package(fckc CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH})
find_package(kll CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH})
find_package(sad CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH})
find_package(hash CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH})

add_executable(${PROJECT_NAME}-test test/main.c)
target_link_libraries(${PROJECT_NAME}-test fck::fckc fck::kll fck::sad)

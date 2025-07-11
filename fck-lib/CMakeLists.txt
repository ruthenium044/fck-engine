cmake_minimum_required(VERSION 3.20)
# fck-library
project(fck-lib)


# Preare modules
set(FCK_WORKSPACE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "Workspace Path")
find_package(fckc CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH})
find_package(kll CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH})
find_package(sad CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH})
find_package(hash CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH})


# Setup pre build pass
add_custom_target(hash-output
     COMMAND "${CMAKE_CURRENT_BINARY_DIR}/hash/fck-hash" "path=${CMAKE_CURRENT_SOURCE_DIR}/../"
     DEPENDS hash) 

add_custom_target(${PROJECT_NAME})

# Setup test
add_executable(${PROJECT_NAME}-test test/main.c)
target_link_libraries(${PROJECT_NAME}-test fck::fckc fck::kll fck::sad fck::hash)


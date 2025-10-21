cmake_minimum_required(VERSION 3.20)
# fck-library
project(fck-lib)

# Preare modules
set(FCK_WORKSPACE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "Workspace Path")
find_package(hash CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH})
find_package(fckc CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH})
find_package(kll CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH})
find_package(std CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH})
find_package(ts CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH})
find_package(api CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH})
#find_package(cnt CONFIG REQUIRED PATHS ${FCK_WORKSPACE_PATH})

add_custom_target(fck-lib-hash-output
     COMMAND "${CMAKE_CURRENT_BINARY_DIR}/hash/fck-hash" 
     "path=${CMAKE_CURRENT_SOURCE_DIR}/test/" 
     "output=${CMAKE_CURRENT_SOURCE_DIR}/hash.txt"
     DEPENDS fck-hash
     BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/hash.txt
     VERBATIM
) 

# Setup test
add_executable(${PROJECT_NAME}-test test/main.c)
target_link_libraries(${PROJECT_NAME}-test fck::fckc fck::kll fck::std fck::hash fck::api)

add_dependencies(${PROJECT_NAME}-test fck-lib-hash-output)

set_target_properties(fck-lib-hash-output PROPERTIES FOLDER "fck-gen")
set_target_properties(${PROJECT_NAME}-test PROPERTIES FOLDER "fck-tests")
cmake_minimum_required(VERSION 3.20)

project(fck-gen)

find_package(fckc)
find_package(os)
find_package(kll)
find_package(hash)

add_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE fck::fckc fck::os fck::kll fck::hash)

find_path(windows_system_file NAMES windows.h)
get_filename_component(windows_system_directory "${windows_system_file}" DIRECTORY)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    fck_gen_target_path="${CMAKE_SOURCE_DIR}"
    fck_gen_system_dir="${windows_system_directory}"
)

target_sources(${PROJECT_NAME} PRIVATE "main.c")

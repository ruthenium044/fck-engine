project(fck-bird-game)

include_guard(GLOBAL)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(${PROJECT_NAME})

find_package(fckc)
# TODO: Should not have reference to input directly - Build an input device with input!
find_package(input)
find_package(os)
find_package(api)
find_package(gameloop)
find_package(ec)
find_package(kll)
find_package(nuklear)
find_package(sprite)

target_link_libraries(${PROJECT_NAME} PUBLIC fck::gameloop)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::kll)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::os)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::api)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::fckc)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::input)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::api-interface)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::nuklear-interface)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::ec-interface)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::sprite-interface)

target_sources(${PROJECT_NAME} PRIVATE "src/fck_bird_game.cpp")

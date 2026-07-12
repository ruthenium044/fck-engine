project(fck-bird-game)

include_guard(GLOBAL)

add_library(${PROJECT_NAME})

find_package(fckc)
find_package(input)
find_package(os)
find_package(api)
find_package(gameloop)
find_package(ec)
find_package(kll)
target_link_libraries(${PROJECT_NAME} PUBLIC fck::gameloop fck::kll fck::os fck::api fck::fckc fck::input fck::api-interface fck::ec-interface)

target_sources(${PROJECT_NAME} PRIVATE "src/fck_bird_game.c")

add_library(ChainedDecosGame STATIC
    Game.cpp
)

target_include_directories(ChainedDecosGame
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
)

add_subdirectory(Map)
add_subdirectory(Menu)
add_subdirectory(Player)
add_subdirectory(GameFeatures)

target_link_libraries(ChainedDecosGame
    PUBLIC
        ChainedDecosEngine
        player
        menu
        mapmod
        GameFeatures
        raylib
        imguilib
        nlohmann_json
)

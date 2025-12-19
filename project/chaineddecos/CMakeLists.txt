set(GAME_SOURCES
    GameApplication.cpp
    GameApplication.h
    GameLayer.cpp
    GameLayer.h
)

add_library(ChainedDecosGame STATIC ${GAME_SOURCES})

target_include_directories(ChainedDecosGame
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/project/chaineddecos
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/project/chaineddecos/Player
        ${CMAKE_SOURCE_DIR}/components/physics/collision
        ${CMAKE_SOURCE_DIR}/scene/resources/model
        ${CMAKE_SOURCE_DIR}/scene/resources/map
        ${CMAKE_SOURCE_DIR}/components/rendering
        ${CMAKE_SOURCE_DIR}/include/imgui
        ${CMAKE_SOURCE_DIR}/include/rlImGui
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/scene/3d/camera
        ${CMAKE_SOURCE_DIR}/scene/main
        ${CMAKE_SOURCE_DIR}/components/physics/dynamics/src
)

# PROJECT_ROOT_DIR is defined globally in the root CMakeLists.txt

add_subdirectory(gamegui)
add_subdirectory(player)


target_link_libraries(ChainedDecosGame
    PUBLIC
        player
        menu
        world
        raylib
        imguilib
        nlohmann_json
)

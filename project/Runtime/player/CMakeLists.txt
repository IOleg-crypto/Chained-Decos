set(PLAYER_SOURCES
    Player.cpp
    Player.h
    PlayerRenderable.cpp
    PlayerRenderable.h
    PlayerInput.cpp
    PlayerInput.h
    PlayerModel.h
    PlayerModel.cpp
    PlayerMovement.h
    PlayerMovement.cpp
    PlayerCollision.cpp
    PlayerCollision.h
    IPlayerInput.h
    IPlayerMovement.h
)

add_library(player STATIC ${PLAYER_SOURCES})

target_include_directories(player PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Engine
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(player PUBLIC
    raylib
    nlohmann_json
    collision
    cameracontroller
    imguilib
    model
    world
)


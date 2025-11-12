add_library(player STATIC
    Player.cpp
    Player.h
    PlayerRenderable.cpp
    PlayerRenderable.h
    Collision/PlayerCollision.cpp
    Collision/PlayerCollision.h
    PlayerInput.cpp
    PlayerInput.h
    Interfaces/IPlayerInput.h
    Interfaces/IPlayerMediator.h
    PlayerModel.h
    PlayerModel.cpp
    PlayerMovement.h
    PlayerMovement.cpp
    Interfaces/IPlayerMovement.h
)

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
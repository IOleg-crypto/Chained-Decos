add_library(player STATIC
    Player.cpp
    Player.h
    PlayerCollision.cpp
    PlayerCollision.h
    PlayerInput.cpp
    PlayerInput.h
    IPlayerInput.h
    PlayerModel.h
    PlayerModel.cpp
    PlayerMovement.h
    PlayerMovement.cpp
    IPlayerMovement.h
)

target_include_directories(player PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
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
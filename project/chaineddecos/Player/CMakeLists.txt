set(PLAYER_SOURCES
    # Core
    Core/Player.cpp
    Core/Player.h
    
    # Components
    Components/PlayerRenderable.cpp
    Components/PlayerRenderable.h
    Components/PlayerInput.cpp
    Components/PlayerInput.h
    Components/PlayerModel.h
    Components/PlayerModel.cpp
    Components/PlayerMovement.h
    Components/PlayerMovement.cpp
    
    # Collision
    Collision/PlayerCollision.cpp
    Collision/PlayerCollision.h
    
    # Interfaces
    Interfaces/IPlayerInput.h
    Interfaces/IPlayerMovement.h
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
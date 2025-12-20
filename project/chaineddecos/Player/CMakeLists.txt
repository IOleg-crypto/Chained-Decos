set(PLAYER_SOURCES
    # Core
    core/player.cpp
    core/player.h
    
    # Components
    components/playerRenderable.cpp
    components/playerRenderable.h
    components/playerInput.cpp
    components/playerInput.h
    components/playerModel.h
    components/playerModel.cpp
    components/playerMovement.h
    components/playerMovement.cpp
    
    # Collision
    collision/playerCollision.cpp
    collision/playerCollision.h
    
    # Interfaces
    interfaces/IPlayerInput.h
    interfaces/IPlayerMovement.h
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


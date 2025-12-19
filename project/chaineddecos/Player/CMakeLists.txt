set(PLAYER_SOURCES
    # Core
    core/Player.cpp
    core/Player.h
    
    # Components
    components/PlayerRenderable.cpp
    components/PlayerRenderable.h
    components/PlayerInput.cpp
    components/PlayerInput.h
    components/PlayerModel.h
    components/PlayerModel.cpp
    components/PlayerMovement.h
    components/PlayerMovement.cpp
    
    # Collision
    collision/PlayerCollision.cpp
    collision/PlayerCollision.h
    
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
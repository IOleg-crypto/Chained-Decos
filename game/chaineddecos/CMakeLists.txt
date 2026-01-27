# Define the Game module
add_library(ChainedDecosGame STATIC)

# Game Sources
target_sources(ChainedDecosGame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_scripts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/playerController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cameraController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/exitScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/playerGui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sceneScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spawnZoneRespawn.h
)

target_link_libraries(ChainedDecosGame PUBLIC engine)
apply_engine_optimizations(ChainedDecosGame)
target_include_directories(ChainedDecosGame PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}
)

# Create the project-specific runtime executable
chained_add_game(ChainedRuntime 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime_entry.cpp
    ChainedDecosGame
)

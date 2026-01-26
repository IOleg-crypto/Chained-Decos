# Define the Game module
add_library(ChainedDecosGame STATIC)

# Game Sources
target_sources(ChainedDecosGame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_scripts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlayerController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CameraController.h
)

target_link_libraries(ChainedDecosGame PUBLIC engine)
apply_engine_optimizations(ChainedDecosGame)
target_include_directories(ChainedDecosGame PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}
)

# Render Module
# Handles all rendering operations

add_library(render
    RenderManager.cpp
    RenderManager.h
)

target_link_libraries(render
    raylib
    imguilib
    collision
    menu
    model
    player
    world
    nlohmann_json
)

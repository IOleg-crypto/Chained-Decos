# Render Module
# Handles all rendering operations

add_library(render STATIC
    RenderManager.cpp
    RenderManager.h
)

target_include_directories(render PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(render PUBLIC
    raylib
    imguilib
    collision
    menu
    model
    player
    world
    nlohmann_json::nlohmann_json
)

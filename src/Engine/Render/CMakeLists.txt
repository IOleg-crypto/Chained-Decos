

add_library(render STATIC
    RenderManager.cpp
    RenderManager.h
    IRenderable.h
    RenderUtils.cpp
    RenderUtils.h
)

target_include_directories(render PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Engine
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(render PUBLIC
    raylib
    imguilib
    collision
    model
    world
    player
    nlohmann_json
)

set_target_properties(render PROPERTIES POSITION_INDEPENDENT_CODE ON)

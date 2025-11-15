

add_library(render STATIC
    Core/RenderManager.cpp
    Core/RenderManager.h
    Interfaces/IGameRenderable.h
    Interfaces/IMenuRenderable.h
    Interfaces/IRenderable.h
    Helpers/ImGuiHelper.cpp
    Helpers/ImGuiHelper.h
    Shader/ShaderManager.cpp
    Shader/ShaderManager.h
    Utils/RenderUtils.cpp
    Utils/RenderUtils.h
    Utils/RenderUtilities.cpp
    Utils/RenderUtilities.h
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

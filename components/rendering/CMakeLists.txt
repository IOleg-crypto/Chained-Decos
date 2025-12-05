cmake_minimum_required(VERSION 3.20)

# Rendering server
add_library(render STATIC
    Core/RenderManager.cpp
    Shader/ShaderManager.cpp
    Utils/RenderUtils.cpp
    Helpers/ImGuiHelper.cpp
)

target_include_directories(render
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/servers/rendering
)

target_link_libraries(render
    PUBLIC
        raylib
        imguilib
        core
        collision
)

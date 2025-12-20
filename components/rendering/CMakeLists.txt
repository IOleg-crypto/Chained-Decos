cmake_minimum_required(VERSION 3.20)

# Rendering server
set(RENDER_SOURCES
    core/RenderManager.cpp
    shader/shaderManager.cpp
    utils/RenderUtils.cpp
    helpers/ImGuiHelper.cpp
)

add_library(render STATIC ${RENDER_SOURCES})

target_include_directories(render
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(render
    PUBLIC
        raylib
        imguilib
        core
        collision
)



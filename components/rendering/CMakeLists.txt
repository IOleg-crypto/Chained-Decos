cmake_minimum_required(VERSION 3.20)

# Rendering server
set(RENDER_SOURCES
    shader/shader_manager.cpp
    utils/render_utils.cpp
    helpers/imgui_helper.cpp
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



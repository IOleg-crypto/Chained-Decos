cmake_minimum_required(VERSION 3.20)

# Add subdirectories for components that still have their own structure
add_subdirectory(config)


# Core library - base systems, events, and module management
set(CORE_SOURCES
    application/application.cpp
    application/application.h
    window/window.cpp
    window/window.h
    layer/layer.cpp
    layer/layer.h
    layer/layer_stack.cpp
    layer/layer_stack.h
    utils/base.h
    utils/types.h
    utils/core.h
    imgui/core/gui_element.h
    imgui/core/gui_manager.h
    imgui/core/gui_manager.cpp
    imgui/imgui_layer.h
    imgui/imgui_layer.cpp
    imgui/components/gui_button.h
    imgui/components/gui_panel.h
    input/input.h
    input/input.cpp
    renderer/renderer.cpp
    renderer/renderer.h
    renderer/renderer_api.cpp
    renderer/renderer_api.h
    renderer/opengl_renderer_api.cpp
    renderer/opengl_renderer_api.h
    renderer/buffer.cpp
    renderer/buffer.h
    renderer/opengl_buffer.cpp
    renderer/opengl_buffer.h
    renderer/vertex_array.cpp
    renderer/vertex_array.h
    renderer/opengl_vertex_array.cpp
    renderer/opengl_vertex_array.h
    renderer/shader.cpp
    renderer/shader.h
    renderer/opengl_shader.cpp
    renderer/opengl_shader.h
    assets/asset_manager.h
    assets/asset_manager.cpp
    audio/audio.cpp
    audio/audio.h
)

add_library(core STATIC ${CORE_SOURCES})

target_include_directories(core
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/include/raylib/src/external/glfw/include
        ${CMAKE_SOURCE_DIR}/include/raylib/src
        ${CMAKE_SOURCE_DIR}/include/raylib/src/external
)

target_link_libraries(core
    PUBLIC
        nlohmann_json
        raylib
        EnTT::EnTT
        config
        ecs
        audio
        render
        collision
        model
        imguilib
        events
        font_resources
        texture_resources
        texture_resources
        world
)

target_compile_definitions(core
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
)



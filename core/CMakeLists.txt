cmake_minimum_required(VERSION 3.20)

# Add subdirectories for components that still have their own structure
add_subdirectory(config)


# Core library - base systems, events, and module management
set(CORE_SOURCES
    application/Application.cpp
    application/Application.h
    window/Window.cpp
    window/Window.h
    layer/Layer.cpp
    layer/Layer.h
    layer/LayerStack.cpp
    layer/LayerStack.h
    module/ModuleManager.cpp
    module/ModuleManager.h
    utils/Base.h
    utils/Types.h
    utils/core.h
    imgui/core/GuiElement.h
    imgui/core/GuiManager.h
    imgui/core/GuiManager.cpp
    imgui/ImGuiLayer.h
    imgui/ImGuiLayer.cpp
    imgui/components/GuiButton.h
    imgui/components/GuiPanel.h
    scripting/ScriptManagerImpl.cpp
    scripting/ScriptManager.h
    scripting/csharp/DotNetHost.cpp
    scripting/csharp/DotNetHost.h
    scripting/csharp/ScriptGlue.cpp
    scripting/csharp/ScriptGlue.h
    input/Input.h
    input/Input.cpp
    renderer/Renderer.cpp
    renderer/Renderer.h
    renderer/RendererAPI.cpp
    renderer/RendererAPI.h
    renderer/OpenGLRendererAPI.cpp
    renderer/OpenGLRendererAPI.h
    renderer/Buffer.cpp
    renderer/Buffer.h
    renderer/OpenGLBuffer.cpp
    renderer/OpenGLBuffer.h
    renderer/VertexArray.cpp
    renderer/VertexArray.h
    renderer/OpenGLVertexArray.cpp
    renderer/OpenGLVertexArray.h
    renderer/Shader.cpp
    renderer/Shader.h
    renderer/OpenGLShader.cpp
    renderer/OpenGLShader.h
    assets/AssetManager.h
    assets/AssetManager.cpp
    physics/Physics.cpp
    physics/Physics.h
    audio/Audio.cpp
    audio/Audio.h
)

add_library(core STATIC ${CORE_SOURCES})

target_include_directories(core
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/include/raylib/src/external/glfw/include
        ${lua_SOURCE_DIR}/src
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
        input
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



cmake_minimum_required(VERSION 3.20)

# Add subdirectories for components that still have their own structure
add_subdirectory(config)


# Core library - base systems, events, and module management
set(CORE_SOURCES
    Engine.cpp
    Engine.h
    application/EngineApplication.cpp
    application/EngineApplication.h
    application/IApplication.h
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
    imgui/ImGuiLayer.h
    imgui/ImGuiLayer.cpp
    imgui/components/GuiButton.h
    imgui/components/GuiPanel.h
    scripting/ScriptManager.cpp
    scripting/ScriptManager.h
    scripting/ScriptEngine.cpp
    scripting/ScriptEngine.h
    input/Input.h
    input/Input.cpp
    renderer/Renderer.cpp
    renderer/Renderer.h
    assets/AssetManager.cpp
    assets/AssetManager.h
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
        liblua
        sol2::sol2
        world
)

target_compile_definitions(core
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
        SOL_ALL_SAFETIES_ON=1
        SOL_USE_CXX17_OPTIONAL=0
)



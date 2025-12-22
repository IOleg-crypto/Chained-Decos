cmake_minimum_required(VERSION 3.20)

# Add subdirectories for components that still have their own structure
add_subdirectory(config)
add_subdirectory(ecs)

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
    events/Event.h
    events/Event.cpp
    events/KeyEvent.h
    events/KeyEvent.cpp
    events/MouseEvent.h
    events/MouseEvent.cpp
    events/ApplicationEvent.h
    events/ApplicationEvent.cpp
    utils/Base.h
    utils/Types.h
    utils/core.h
    gui/core/GuiElement.h
    gui/core/GuiManager.h
    gui/ImGuiLayer.h
    gui/ImGuiLayer.cpp
    gui/components/GuiButton.h
    gui/components/GuiPanel.h
)

add_library(core STATIC ${CORE_SOURCES})

target_include_directories(core
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/include/raylib/src/external/glfw/include
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
)



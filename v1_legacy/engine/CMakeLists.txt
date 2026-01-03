add_library(engine STATIC
    core/application/application.cpp
    core/application/application.h
    core/application/entry_point.h
    core/window/window.cpp
    core/window/window.h
    core/input/input.cpp
    core/input/input.h
    core/layer/layer.cpp
    core/layer/layer.h
    core/layer/layer_stack.cpp
    core/layer/layer_stack.h
    core/engine_layer.cpp
    core/engine_layer.h
    gui/gui_manager.cpp
    gui/gui_manager.h
    gui/gui_element.h
    gui/gui_button.h
    gui/gui_panel.h
    gui/imgui_layer.cpp
    gui/imgui_layer.h
    assets/asset_manager.cpp
    assets/asset_manager.h
    config/config_manager.cpp
    config/config_manager.h
    config/game_config.cpp
    config/game_config.h
)

# Add subdirectories for other engine modules
add_subdirectory(renderer)
add_subdirectory(audio)
add_subdirectory(physics)
add_subdirectory(scene)
add_subdirectory(scripting)

target_include_directories(engine PUBLIC
    .
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/engine
)

target_link_libraries(engine PUBLIC
    core
    events
    scene_core
    collision_core
    physics_core
    audio_core
    renderer_core
    imguilib
)

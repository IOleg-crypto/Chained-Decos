
add_library(editor_core STATIC)

target_sources(editor_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/editor_layer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/editor_layer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor_panels.h
    ${CMAKE_CURRENT_SOURCE_DIR}/editor_panels.cpp

    # Project
    ${CMAKE_CURRENT_SOURCE_DIR}/project/editor_settings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/project/project_serializer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/project/project_serializer.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/editor_project_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/editor_project_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor_scene_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/editor_scene_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_picking.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_picking.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_factory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_factory.cpp

    # Viewport
    ${CMAKE_CURRENT_SOURCE_DIR}/viewport/editor_gizmo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/viewport/editor_gizmo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewport/editor_camera.h
    ${CMAKE_CURRENT_SOURCE_DIR}/viewport/editor_camera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewport/ui_manipulator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/viewport/ui_manipulator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/viewport/ui_manipulator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor_asset_types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/editor_action_commands.h
    ${CMAKE_CURRENT_SOURCE_DIR}/editor_action_commands.cpp

    # Panels
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/scene_hierarchy_panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/scene_hierarchy_panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/inspector_panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/property_editor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/property_editor.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/content_browser_panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/content_browser_panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/content_browser_provider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/content_browser_provider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/console_panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/console_panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/profiler_panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/profiler_panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/viewport_panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/project_settings_panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/project_settings_panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/world_panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/world_panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/effects_panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/effects_panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/material_panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/material_panel.cpp
    

    # Undo
    ${CMAKE_CURRENT_SOURCE_DIR}/undo/command_history.h
    ${CMAKE_CURRENT_SOURCE_DIR}/undo/command_history.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/undo/editor_command.h
    ${CMAKE_CURRENT_SOURCE_DIR}/undo/modify_component_command.h
    ${CMAKE_CURRENT_SOURCE_DIR}/undo/lambda_command.h
    ${CMAKE_CURRENT_SOURCE_DIR}/undo/entity_commands.h

    # UI
    ${CMAKE_CURRENT_SOURCE_DIR}/editor_gui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/editor_gui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor_layout.h
    ${CMAKE_CURRENT_SOURCE_DIR}/editor_layout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/project_selector_ui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/project_selector_ui.cpp

    # Utilities
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/editor_utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/editor_utils.cpp
)


target_link_libraries(editor_core PUBLIC
    ChainedEngine::Framework
    engine_external_imgui
    nfd
    assimp
    Coral.Native
)

target_include_directories(editor_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Define the Editor executable
add_executable(ChainedEditor ${CMAKE_CURRENT_SOURCE_DIR}/editor_main.cpp
        editor_icons.h)
target_link_libraries(ChainedEditor PRIVATE editor_core ChainedDecos)
target_include_directories(ChainedEditor PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# Apply optimizations
if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(editor_core)
    apply_engine_optimizations(ChainedEditor)
endif()

# Installation
install(TARGETS ChainedEditor editor_core
    RUNTIME DESTINATION bin COMPONENT Runtime
    ARCHIVE DESTINATION lib COMPONENT Runtime
    LIBRARY DESTINATION lib COMPONENT Runtime
)

# Copy global assets to the install directory if they exist
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/" DESTINATION assets COMPONENT Runtime)
endif()

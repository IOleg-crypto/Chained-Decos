
add_library(editor_core STATIC)

target_sources(editor_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/layer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/layer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels.h
    ${CMAKE_CURRENT_SOURCE_DIR}/panels.cpp

    # Project
    ${CMAKE_CURRENT_SOURCE_DIR}/project/editor_settings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/project/project_serializer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/project/project_serializer.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/project_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/project_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_picking.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_picking.cpp

    # Viewport
    ${CMAKE_CURRENT_SOURCE_DIR}/viewport/gizmo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/viewport/gizmo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewport/camera.h
    ${CMAKE_CURRENT_SOURCE_DIR}/viewport/camera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewport/ui_manipulator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/viewport/ui_manipulator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/viewport/ui_manipulator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/asset_types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/action_commands.h
    ${CMAKE_CURRENT_SOURCE_DIR}/action_commands.cpp

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
    ${CMAKE_CURRENT_SOURCE_DIR}/undo/command.h
    ${CMAKE_CURRENT_SOURCE_DIR}/undo/modify_component_command.h
    ${CMAKE_CURRENT_SOURCE_DIR}/undo/lambda_command.h
    ${CMAKE_CURRENT_SOURCE_DIR}/undo/entity_commands.h

    # UI
    ${CMAKE_CURRENT_SOURCE_DIR}/gui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/gui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/layout.h
    ${CMAKE_CURRENT_SOURCE_DIR}/layout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/project_selector_ui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/project_selector_ui.cpp

    # Utilities
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_properties.cpp
)


target_link_libraries(editor_core PUBLIC
    ChainedEngine::Framework
    engine_external_imgui
    nfd
    assimp
    Coral.Native
    engine_core
)

target_include_directories(editor_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Define the Editor executable
add_executable(ChainedEditor ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        icons.h)
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

# Automatically sync resources/ into the correct bin/<Config>/ directory after each build
if(COMMAND ch_add_resource_sync)
    ch_add_resource_sync(ChainedEditor)
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

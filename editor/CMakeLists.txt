# Define the Editor executable
add_executable(ChainedEditor)

# Main Editor Sources
target_sources(ChainedEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/editor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/editor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor_layer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/editor_layer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor_panels.h
    ${CMAKE_CURRENT_SOURCE_DIR}/editor_panels.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/actions/project_actions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/actions/scene_actions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/actions/editor_actions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/actions/editor_actions.cpp

    # Viewport
    ${CMAKE_CURRENT_SOURCE_DIR}/viewport/editor_gizmo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/viewport/editor_gizmo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/viewport/editor_camera.h
    ${CMAKE_CURRENT_SOURCE_DIR}/viewport/editor_camera.cpp

    # Panels
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/scene_hierarchy_panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/scene_hierarchy_panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/inspector_panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/property_editor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/property_editor.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/content_browser_panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/content_browser_panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/console_panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/console_panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/profiler_panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/profiler_panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/viewport_panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/project_browser_panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/project_settings_panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/project_settings_panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/environment_panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/environment_panel.cpp

    # Undo
    ${CMAKE_CURRENT_SOURCE_DIR}/undo/command_history.h
    ${CMAKE_CURRENT_SOURCE_DIR}/undo/command_history.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/undo/editor_command.h
    ${CMAKE_CURRENT_SOURCE_DIR}/undo/modify_component_command.h
    ${CMAKE_CURRENT_SOURCE_DIR}/undo/lambda_command.h
    ${CMAKE_CURRENT_SOURCE_DIR}/undo/entity_commands.h

    # UI
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/editor_gui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/editor_gui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/editor_layout.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/editor_layout.cpp
    # External
    ${CMAKE_SOURCE_DIR}/include/rlImGui/rlimgui.cpp
)

target_link_libraries(ChainedEditor PUBLIC engine ChainedDecosGame nfd)
apply_engine_optimizations(ChainedEditor)
target_include_directories(ChainedEditor PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# Copy resources to build directory
if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
    file(COPY "${CMAKE_SOURCE_DIR}/resources" DESTINATION "${CMAKE_BINARY_DIR}")
endif()

# Copy assets to build directory
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    file(COPY "${CMAKE_SOURCE_DIR}/assets" DESTINATION "${CMAKE_BINARY_DIR}")
endif()

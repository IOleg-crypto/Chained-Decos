# Editor Core Library (Static) for testability
add_library(editor_core STATIC)

target_sources(editor_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/editor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/editor.cpp
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


target_link_libraries(editor_core PUBLIC 
    engine
    ChainedDecosGame
    nfd
)
target_include_directories(editor_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/engine
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/rlImGui
    ${imguizmo_SOURCE_DIR}
)

# Define the Editor executable
add_executable(ChainedEditor ${CMAKE_CURRENT_SOURCE_DIR}/editor_main.cpp)
target_link_libraries(ChainedEditor PRIVATE editor_core)
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
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

# Copy global resources/assets to the install directory
if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/" DESTINATION resources)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/" DESTINATION assets)
endif()

set(EDITOR_SOURCES
    editor.h
    editor.cpp
    editor_main.cpp
    editor_layer.h
    editor_layer.cpp
    viewport/editor_gizmo.h
    viewport/editor_gizmo.cpp
    viewport/editor_camera.h
    viewport/editor_camera.cpp
    panels/scene_hierarchy_panel.h
    panels/scene_hierarchy_panel.cpp
    panels/inspector_panel.h
    panels/inspector_panel.cpp
    logic/undo/command_history.cpp
    logic/undo/command_history.h
    logic/undo/editor_command.h
    logic/undo/transform_command.h
    logic/undo/entity_commands.h
    panels/content_browser_panel.h
    panels/content_browser_panel.cpp
    panels/console_panel.h
    panels/console_panel.cpp
    panels/viewport_panel.cpp
    panels/project_browser_panel.cpp
    panels/viewport_panel.h
    panels/environment_panel.h
    panels/environment_panel.cpp
    ui/toolbar.h
    ui/toolbar.cpp
    ui/project_selector.h
    ui/project_selector.cpp
)

add_executable(ChainedEditor ${EDITOR_SOURCES})

target_link_libraries(ChainedEditor PUBLIC engine nfd)
target_include_directories(ChainedEditor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Copy resources to build directory
if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
    file(COPY "${CMAKE_SOURCE_DIR}/resources" DESTINATION "${CMAKE_BINARY_DIR}")
endif()

# Copy assets to build directory
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    file(COPY "${CMAKE_SOURCE_DIR}/assets" DESTINATION "${CMAKE_BINARY_DIR}")
endif()

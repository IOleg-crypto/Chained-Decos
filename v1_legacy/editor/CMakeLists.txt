
# ChainedEditor - Engine Editor for Chained Decos

set(logic_sources
    logic/editor_settings.cpp
    logic/editor_settings.h
    logic/editor_scene_manager.cpp
    logic/editor_scene_manager.h
    logic/editor_scene_actions.cpp
    logic/editor_scene_actions.h
    logic/editor_project_actions.cpp
    logic/editor_project_actions.h
    logic/editor_entity_factory.cpp
    logic/editor_entity_factory.h
    logic/editor_input.cpp
    logic/editor_input.h
    logic/selection_manager.cpp
    logic/selection_manager.h
    logic/project_manager.cpp
    logic/project_manager.h
    logic/scene_simulation_manager.cpp
    logic/scene_simulation_manager.h
    logic/scene_cloner.h
    logic/panel_manager.h
    logic/undo/editor_command.h
    logic/undo/command_history.cpp
    logic/undo/command_history.h
)

set(panels_sources
    panels/editor_panel.h
    panels/hierarchy_panel.cpp
    panels/hierarchy_panel.h
    panels/inspector_panel.cpp
    panels/inspector_panel.h
    panels/viewport_panel.cpp
    panels/viewport_panel.h
    panels/console_panel.cpp
    panels/console_panel.h
    panels/toolbar_panel.cpp
    panels/toolbar_panel.h
    panels/menu_bar_panel.cpp
    panels/menu_bar_panel.h
    panels/project_browser_panel.cpp
    panels/project_browser_panel.h
    panels/content_browser_panel.cpp
    panels/content_browser_panel.h
    utils/editor_grid.cpp
    utils/editor_grid.h
    utils/process_utils.cpp
    utils/process_utils.h
    viewport/viewport_picking.cpp
    viewport/viewport_picking.h
    viewport/viewport_gizmo.cpp
    viewport/viewport_gizmo.h
    viewport/viewport_renderer.cpp
    viewport/viewport_renderer.h
    camera/editor_camera.cpp
    camera/editor_camera.h
)

set(editor_sources
    editor_application.cpp
    editor_application.h
    editor_layer.cpp
    editor_layer.h
)

add_library(editor_logic STATIC ${logic_sources})
target_include_directories(editor_logic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/json/include
    ${CMAKE_SOURCE_DIR}/include/entt/src
    ${CMAKE_SOURCE_DIR}/include/nfd/src/include
)
target_link_libraries(editor_logic PUBLIC 
    raylib
    core
    nfd
    nlohmann_json::nlohmann_json
    map
    project
)

add_library(editor_panels STATIC ${panels_sources})
target_include_directories(editor_panels PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/json/include
    ${CMAKE_SOURCE_DIR}/include/entt/src
    ${CMAKE_SOURCE_DIR}/include/nfd/src/include
)
target_link_libraries(editor_panels PUBLIC 
    raylib
    core
    nlohmann_json::nlohmann_json
    cameracontroller
    editor_logic
    project
)

add_library(editor_core STATIC ${editor_sources})
target_include_directories(editor_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/json/include
    ${CMAKE_SOURCE_DIR}/include/entt/src
    ${CMAKE_SOURCE_DIR}/include/nfd/src/include
)
target_link_libraries(editor_core PUBLIC 
    raylib
    core
    engine
    nlohmann_json::nlohmann_json
    cameracontroller
    editor_panels
    RuntimeLib
    project
)

add_executable(ChainedEditor
    main.cpp 
)

target_link_libraries(ChainedEditor PUBLIC 
    editor_core
)

target_compile_definitions(ChainedEditor PRIVATE CHAINEDEDITOR_EXPORTS)

set_target_properties(ChainedEditor PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

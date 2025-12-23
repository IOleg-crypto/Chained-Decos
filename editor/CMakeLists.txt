
# ChainedEditor - Engine Editor for Chained Decos



# UI and legacy map editing components
set(mapgui_sources
    mapgui/UIManager.cpp
    mapgui/UIManager.h
    mapgui/skyboxBrowser.cpp
    mapgui/skyboxBrowser.h
)

set(tool_sources
    tool/IToolManager.h
    tool/ToolManager.cpp
    tool/ToolManager.h
)

set(render_sources
    render/EditorRenderer.cpp
    render/EditorRenderer.h
)

set(logic_sources
    logic/MapManager.cpp
    logic/MapManager.h
    logic/ProjectManager.cpp
    logic/ProjectManager.h
    logic/ProjectData.h
    logic/EditorSettings.h
    logic/Scene.cpp
    logic/Scene.h
    logic/ISceneManager.h
    logic/SceneManager.cpp
    logic/SceneManager.h
    logic/ISelectionManager.h
    logic/SelectionManager.cpp
    logic/SelectionManager.h
    logic/IEditorState.h
    logic/EditorState.cpp
    logic/EditorState.h
)

# New panel-based UI system
set(panels_sources
    panels/IEditorPanel.h
    panels/EditorPanelManager.cpp
    panels/EditorPanelManager.h
    panels/HierarchyPanel.cpp
    panels/HierarchyPanel.h
    panels/InspectorPanel.cpp
    panels/InspectorPanel.h
    panels/ViewportPanel.cpp
    panels/ViewportPanel.h
    panels/ConsolePanel.cpp
    panels/ConsolePanel.h
    panels/AssetBrowserPanel.cpp
    panels/AssetBrowserPanel.h
    panels/ToolbarPanel.cpp
    panels/ToolbarPanel.h
    panels/UIEditorPanel.cpp
    panels/UIEditorPanel.h
)

set(editor_sources
    EditorApplication.cpp
    EditorApplication.h
    Editor.cpp
    Editor.h
)

add_executable(ChainedEditor
    ${editor_sources}
    ${panels_sources}
    ${render_sources}
    ${tool_sources}
    ${mapgui_sources}
    ${logic_sources}
)

target_include_directories(ChainedEditor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/json/include
    ${CMAKE_SOURCE_DIR}/include/nfd/src/include
)

target_link_libraries(ChainedEditor PUBLIC 
    raylib
    core
    nfd
    nlohmann_json::nlohmann_json
    cameracontroller
    model
    map
    ChainedDecosGame
)

target_compile_definitions(ChainedEditor PRIVATE CHAINEDEDITOR_EXPORTS)

set_target_properties(ChainedEditor PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

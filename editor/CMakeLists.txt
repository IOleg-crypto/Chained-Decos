
# ChainedEditor - Engine Editor for Chained Decos

set(logic_sources
    logic/MapManager.cpp
    logic/MapManager.h
    logic/EditorSettings.cpp
    logic/EditorSettings.h
    logic/SceneManager.cpp
    logic/SceneManager.h
    logic/SelectionManager.cpp
    logic/SelectionManager.h
    logic/ProjectManager.cpp
    logic/ProjectManager.h
    logic/SceneSimulationManager.cpp
    logic/SceneSimulationManager.h
    logic/EditorState.cpp
    logic/EditorState.h
    logic/SceneCloner.h
    logic/undo/EditorCommand.h
    logic/undo/CommandHistory.cpp
    logic/undo/CommandHistory.h
    logic/undo/TransformCommand.cpp
    logic/undo/TransformCommand.h
    logic/undo/AddObjectCommand.cpp
    logic/undo/AddObjectCommand.h
    logic/undo/ModifyObjectCommand.h
    logic/undo/DeleteObjectCommand.cpp
    logic/undo/DeleteObjectCommand.h
)

set(panels_sources
    panels/IEditorPanel.h
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
    panels/MenuBarPanel.cpp
    panels/MenuBarPanel.h
    panels/ProjectBrowserPanel.cpp
    panels/ProjectBrowserPanel.h
    utils/EditorGrid.cpp
    utils/EditorGrid.h
    viewport/ViewportPicking.cpp
    viewport/ViewportPicking.h
    viewport/ViewportGizmo.cpp
    viewport/ViewportGizmo.h
    viewport/ViewportRenderer.cpp
    viewport/ViewportRenderer.h
    camera/EditorCamera.cpp
    camera/EditorCamera.h
)

set(editor_sources
    EditorApplication.cpp
    EditorApplication.h
    EditorLayer.cpp
    EditorLayer.h
)

add_library(editor_logic STATIC ${logic_sources})
target_include_directories(editor_logic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/json/include
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
    ${CMAKE_SOURCE_DIR}/include/nfd/src/include
)
target_link_libraries(editor_core PUBLIC 
    raylib
    core
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

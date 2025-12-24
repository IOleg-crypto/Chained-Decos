
# ChainedEditor - Engine Editor for Chained Decos

set(logic_sources
    logic/MapManager.cpp
    logic/MapManager.h
    logic/ProjectData.cpp
    logic/ProjectData.h
    logic/EditorSettings.cpp
    logic/EditorSettings.h
    logic/Scene.cpp
    logic/Scene.h
    logic/SceneManager.cpp
    logic/SceneManager.h
    logic/SelectionManager.cpp
    logic/SelectionManager.h
    logic/EditorState.cpp
    logic/EditorState.h
    logic/SceneCloner.h
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
)

add_executable(ChainedEditor
    ${editor_sources}
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
    editor_panels
)

target_compile_definitions(ChainedEditor PRIVATE CHAINEDEDITOR_EXPORTS)

set_target_properties(ChainedEditor PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

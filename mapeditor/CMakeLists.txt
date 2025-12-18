
set(mapeditor_entry_point main.cpp)

set(mapgui_sources
    mapgui/UIManager.cpp
    mapgui/UIManager.h
    mapgui/SkyboxBrowser.cpp
    mapgui/SkyboxBrowser.h
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

set(editor_sources
    EditorApplication.cpp
    EditorApplication.h
    Editor.cpp
    Editor.h
)

add_executable(map_editor
    ${editor_sources}
    ${render_sources}
    ${tool_sources}
    ${mapgui_sources}
    ${mapeditor_entry_point}
)
target_include_directories(map_editor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/json/include
    ${CMAKE_SOURCE_DIR}/include/nfd/src/include
)

target_link_libraries(map_editor PUBLIC 
    raylib
    core
    nfd
    nlohmann_json::nlohmann_json
    cameracontroller
    model
    map
    ChainedDecosGame
)

target_compile_definitions(map_editor PRIVATE CHAINEDDECOSEDITOR_EXPORTS)

set_target_properties(map_editor PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

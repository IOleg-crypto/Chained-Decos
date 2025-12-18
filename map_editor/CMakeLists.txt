add_executable(map_editor
    Modules/EditorModule.cpp
    Modules/EditorModule.h
    EditorApplication.cpp
    EditorApplication.h
    Editor/Editor.cpp
    Editor/Editor.h
    Editor/Renderer/EditorRenderer.cpp
    Editor/ToolManager/ToolManager.cpp
    Editor/UIManager/ObjectFactory.cpp
    Editor/UIManager/SkyboxBrowser.cpp
    Editor/UIManager/UIManager.cpp
    Editor/Utils/PathUtils.cpp
    main.cpp
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

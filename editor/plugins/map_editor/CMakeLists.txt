add_executable(map_editor
    Modules/EditorModule.cpp
    Modules/EditorModule.h
    EditorApplication.cpp
    EditorApplication.h
    Editor/Editor.cpp
    Editor/Editor.h
    Editor/CameraManager/CameraManager.cpp
    Editor/FileManager/FileManager.cpp
    Editor/FileManager/MapObjectConverterEditor.cpp
    Editor/ModelManager/ModelManager.cpp
    Editor/Object/MapObject.cpp
    Editor/Renderer/EditorRenderer.cpp
    Editor/SceneManager/BoundingBoxCalculators.cpp
    Editor/SceneManager/SceneManager.cpp
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
)

target_link_libraries(map_editor PUBLIC 
    raylib
    ChainedDecosEngine
    nfd
    nlohmann_json::nlohmann_json
    cameracontroller
    model
    map
)

target_compile_definitions(map_editor PRIVATE CHAINEDDECOSEDITOR_EXPORTS)

set_target_properties(map_editor PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

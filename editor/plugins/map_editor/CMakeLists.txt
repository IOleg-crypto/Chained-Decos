add_library(map_editor SHARED
    Modules/EditorModule.cpp
    Modules/EditorModule.h
    EditorApplication.cpp
    EditorApplication.h
    Editor/Editor.cpp
    Editor/Editor.h
    main.cpp
)

target_include_directories(map_editor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui
)

target_link_libraries(map_editor PUBLIC 
    raylib
    kernel
    engine
)

target_compile_definitions(map_editor PRIVATE CHAINEDDECOSEDITOR_EXPORTS)

set_target_properties(map_editor PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(editor STATIC
    editor.h
    editor.cpp
    editor_layer.h
    editor_layer.cpp
    scene_hierarchy_panel.h
    scene_hierarchy_panel.cpp
    inspector_panel.h
    inspector_panel.cpp
)

target_link_libraries(editor PUBLIC engine nfd)
target_include_directories(editor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

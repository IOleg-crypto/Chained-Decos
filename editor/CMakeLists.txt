add_library(editor STATIC
    editor.h
    editor.cpp
    editor_layer.h
    editor_layer.cpp
)

target_link_libraries(editor PUBLIC engine)
target_include_directories(editor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

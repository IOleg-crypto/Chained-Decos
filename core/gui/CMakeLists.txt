add_library(core_gui STATIC
    GUIContext.cpp
    GUIContext.h
    GUIStyle.h
    layout/GUILayout.cpp
    layout/GUILayout.h
    controls/Button.h
    controls/Label.h
    controls/Panel.h
)

target_include_directories(core_gui
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/include/raylib/src/external/glfw/include
)

target_link_libraries(core_gui
    PUBLIC
        raylib
)

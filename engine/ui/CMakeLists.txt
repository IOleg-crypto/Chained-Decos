# Register UI sources to the unified engine target
target_sources(engine PRIVATE
    # UI Rendering
    ${CMAKE_CURRENT_SOURCE_DIR}/canvas_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/canvas_renderer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/font_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/font_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_raylib_ui.h
)

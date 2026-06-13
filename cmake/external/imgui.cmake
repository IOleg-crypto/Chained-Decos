# ImGui + ImGuizmo dependency
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/include/imgui")
set(IMGUIZMO_DIR "${CMAKE_SOURCE_DIR}/include/imguizmo")

add_library(engine_external_imgui STATIC
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
    "${IMGUIZMO_DIR}/ImGuizmo.cpp"
    "${IMGUIZMO_DIR}/GraphEditor.cpp"
    "${IMGUIZMO_DIR}/ImCurveEdit.cpp"
    "${IMGUIZMO_DIR}/ImGradient.cpp"
    "${IMGUIZMO_DIR}/ImSequencer.cpp"
)

target_include_directories(engine_external_imgui PUBLIC 
    "${IMGUI_DIR}" 
    "${IMGUIZMO_DIR}" 
    "${IMGUI_DIR}/backends"
)

# ImGui depends on GLFW and GLAD for its backends
target_link_libraries(engine_external_imgui 
    PUBLIC 
        glfw 
        engine_external_glad
)

target_compile_definitions(engine_external_imgui PUBLIC 
    IMGUI_DEFINE_MATH_OPERATORS 
    GLFW_INCLUDE_NONE 
    IMGUI_IMPL_OPENGL_LOADER_GLAD
)

set_target_properties(engine_external_imgui PROPERTIES UNITY_BUILD OFF)

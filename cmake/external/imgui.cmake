# ImGui + ImGuizmo dependency
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/thirdparty/imgui")
set(IMGUIZMO_DIR "${CMAKE_SOURCE_DIR}/thirdparty/imguizmo")

set(IMGUI_PLATFORM_SOURCES "")
set(IMGUI_PLATFORM_DEPS "")

if(CH_PLATFORM_BACKEND STREQUAL "sdl3")
    list(APPEND IMGUI_PLATFORM_SOURCES
        "${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp"
    )
    set(IMGUI_PLATFORM_DEPS SDL3-static)
else()
    list(APPEND IMGUI_PLATFORM_SOURCES
        "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    )
    set(IMGUI_PLATFORM_DEPS glfw)
endif()

add_library(engine_external_imgui STATIC
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp"
    ${IMGUI_PLATFORM_SOURCES}
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

target_link_libraries(engine_external_imgui 
    PUBLIC 
        ${IMGUI_PLATFORM_DEPS}
        engine_external_glad
)

if(UNIX AND NOT APPLE)
    target_link_libraries(engine_external_imgui PUBLIC X11)
endif()

target_compile_definitions(engine_external_imgui PUBLIC 
    IMGUI_DEFINE_MATH_OPERATORS
    IMGUI_IMPL_OPENGL_LOADER_GLAD
)

if(CH_PLATFORM_BACKEND STREQUAL "sdl3")
    target_compile_definitions(engine_external_imgui PUBLIC CH_PLATFORM_BACKEND_SDL3)
else()
    target_compile_definitions(engine_external_imgui PUBLIC GLFW_INCLUDE_NONE CH_PLATFORM_BACKEND_GLFW)
endif()

set_target_properties(engine_external_imgui PROPERTIES UNITY_BUILD OFF)

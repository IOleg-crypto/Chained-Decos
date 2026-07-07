chained_add_engine_module(engine_platform_glfw
    SOURCES
        glfw_window.cpp
        glfw_window.h
    DEPENDS
        engine_core
        glfw
        engine_external_glad
        engine_external_imgui
)

target_compile_definitions(engine_platform_glfw PRIVATE GLFW_INCLUDE_NONE CH_PLATFORM_BACKEND_GLFW)

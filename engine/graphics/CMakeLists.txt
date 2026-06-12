set(STB_IMPL_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/stb_impl.cpp")
file(WRITE "${STB_IMPL_SOURCE}" "#define STB_IMAGE_IMPLEMENTATION\n#include \"stb_image.h\"\n")

chained_add_engine_module(engine_graphics
    SOURCES
        ${STB_IMPL_SOURCE}
    # Pipeline
        pipeline/renderer.cpp
        pipeline/renderer2d.cpp
        pipeline/renderer3d.cpp
        pipeline/debug_renderer.cpp
        pipeline/renderer_types.h
        pipeline/shader_library.cpp
        pipeline/scene_renderer.cpp
        pipeline/passes/geometry_pass.cpp
        pipeline/passes/skybox_pass.cpp
        pipeline/passes/shadow_pass.cpp
        pipeline/passes/composite_pass.cpp
        pipeline/render_command.cpp
        pipeline/texture_utility.cpp
        pipeline/geometry_generator.cpp
        pipeline/renderer.h
        pipeline/renderer2d.h
        pipeline/renderer3d.h
        pipeline/debug_renderer.h
        pipeline/scene_renderer.h

        pipeline/shader_library.h
        pipeline/material.h
        pipeline/frustum.h
        pipeline/texture_utility.h
        pipeline/geometry_generator.h
    # Loaders module moved to loaders/CMakeLists.txt
    # API & Backend
        api/renderer_api.cpp
        api/buffer.cpp
        api/buffer.h
        api/vertex_array.cpp
        api/vertex_array.h
        api/texture.cpp
        api/texture.h
        api/framebuffer.cpp
        api/framebuffer.h
        api/renderer_api.h
        api/model_data.h
        api/storage_buffer.h
        api/storage_buffer.cpp
        api/shader.h
        api/shader.cpp
        api/opengl/opengl_renderer_api.cpp
        api/opengl/opengl_renderer_api.h
        api/opengl/opengl_buffer.cpp
        api/opengl/opengl_buffer.h
        api/opengl/opengl_vertex_array.cpp
        api/opengl/opengl_vertex_array.h
        api/opengl/opengl_texture.cpp
        api/opengl/opengl_texture.h
        api/opengl/opengl_framebuffer.cpp
        api/opengl/opengl_framebuffer.h
        api/opengl/opengl_shader.h
        api/opengl/opengl_shader.cpp
        api/opengl/opengl_uniform_buffer.h
        api/opengl/opengl_uniform_buffer.cpp
        api/opengl/opengl_storage_buffer.h

        api/opengl/opengl_storage_buffer.cpp
    # UI
        ui/ui_renderer.cpp
        ui/ui_widget_renderer.cpp
        ui/ui_factory.cpp
        ui/ui_font_registry.cpp
        ui/ui_layout_system.cpp
        ui/ui_animation_system.cpp
        ui/ui_input_system.cpp
        ui/ui_renderer.h
        ui/ui_widget_renderer.h
        ui/ui_factory.h
        ui/ui_font_registry.h
        ui/ui_layout_system.h
        ui/ui_animation_system.h
        ui/ui_input_system.h
        ui/ui_style.h
        ui/ui_types.h
        ui/ui_data_components.h
    DEPENDS
        engine_core
        engine_audio
        engine_assets
        ChainedEngine::External::YamlCpp
        ChainedEngine::External::EnTT
        ChainedEngine::External::GLM
        ChainedEngine::External::STB
        ChainedEngine::External::ImGui
        ChainedEngine::External::GLFW
        ChainedEngine::External::GLAD
        ChainedEngine::External::Assimp
)

# Prevents multiple definition errors when using unity builds
set_source_files_properties(${STB_IMPL_SOURCE} PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)

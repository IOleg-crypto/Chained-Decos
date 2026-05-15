set(STB_IMPL_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/stb_impl.cpp")
file(WRITE "${STB_IMPL_SOURCE}" "#define STB_IMAGE_IMPLEMENTATION\n#include \"stb_image.h\"\n")

chained_add_engine_module(engine_graphics
    SOURCES
        ${STB_IMPL_SOURCE}
    # Pipeline
        pipeline/renderer.cpp
        pipeline/renderer_types.cpp
        pipeline/ui_renderer.cpp
        pipeline/ui_widget_renderer.cpp
        pipeline/ui_font_registry.cpp
        pipeline/shader_library.cpp
        pipeline/scene_renderer.cpp
        pipeline/render_command.cpp
        pipeline/texture_utility.cpp
        texture_system.cpp
        pipeline/geometry_generator.cpp
        pipeline/renderer.h
        pipeline/scene_renderer.h
        pipeline/ui_renderer.h
        pipeline/ui_style.h
        pipeline/ui_widget_renderer.h
        pipeline/ui_font_registry.h
        pipeline/shader_library.h
        pipeline/material.h
        pipeline/frustum.h
        pipeline/texture_utility.h
        texture_system.h
        pipeline/geometry_generator.h
    # Assets
        assets/shader_asset.cpp
        assets/model_asset.cpp
        assets/texture_asset.cpp
        assets/font_asset.cpp
        assets/shader_asset.h
        assets/model_asset.h
        assets/texture_asset.h
        assets/font_asset.h
        assets/environment.h
    # Loaders
        loaders/texture_loader.cpp
        loaders/model_loader.cpp
        loaders/model_cache.cpp
        loaders/assimp_importer.cpp
        loaders/shader_loader.cpp
        loaders/font_loader.cpp
        loaders/environment_loader.cpp
        loaders/texture_loader.h
        loaders/model_loader.h
        loaders/model_cache.h
        loaders/assimp_importer.h
        loaders/shader_loader.h
        loaders/font_loader.h
        loaders/environment_loader.h
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
    DEPENDS
        engine_core
        engine_audio
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

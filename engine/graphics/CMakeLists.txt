add_library(engine_graphics ${SUBMODULE_LIB_TYPE}
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
        pipeline/geometry_generator.cpp
        pipeline/renderer.h
        pipeline/scene_renderer.h
        pipeline/ui_renderer.h
        pipeline/ui_widget_renderer.h
        pipeline/ui_font_registry.h
        pipeline/shader_library.h
        pipeline/material.h
        pipeline/frustum.h
        pipeline/texture_utility.h
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
        loaders/assimp_importer.cpp
        loaders/shader_loader.cpp
        loaders/font_loader.cpp
        loaders/environment_loader.cpp
        loaders/texture_loader.h
        loaders/model_loader.h
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
)

target_include_directories(engine_graphics PUBLIC
    ${ENGINE_INCLUDE_DIRS}
    ${assimp_SOURCE_DIR}/include
    ${assimp_BINARY_DIR}/include
    ${yaml-cpp_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/imgui/backends
    ${CMAKE_SOURCE_DIR}/include/entt/src
    ${CMAKE_SOURCE_DIR}/include/stb
    ${imguizmo_SOURCE_DIR}
)

target_link_libraries(engine_graphics PUBLIC 
    engine_core
    imguilib 
    assimp::assimp 
    yaml-cpp
    EnTT::EnTT
    glfw
    glad
)

if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(engine_graphics)
endif()

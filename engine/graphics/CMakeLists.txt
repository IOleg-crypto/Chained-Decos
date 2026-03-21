add_library(engine_graphics OBJECT
    # Pipeline
    pipeline/renderer.cpp
    pipeline/ui_renderer.cpp
    pipeline/shader_library.cpp
    pipeline/scene_renderer.cpp
    pipeline/renderer2d.cpp
    pipeline/render_command.cpp
    pipeline/renderer.h
    pipeline/scene_renderer.h
    pipeline/ui_renderer.h
    pipeline/shader_library.h
    pipeline/material.h
    pipeline/frustum.h

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

    # Importers
    importers/mesh_importer.cpp
    importers/texture_importer.cpp
    importers/shader_importer.cpp
    importers/font_importer.cpp
    importers/environment_importer.cpp
    importers/mesh_importer.h
    importers/texture_importer.h
    importers/shader_importer.h
    importers/font_importer.h
    importers/asset_importer.h

    # Loaders
    loaders/texture_loader.h
    loaders/model_loader.h
    loaders/shader_loader.h
    loaders/font_loader.h
    loaders/audio_loader.h
    loaders/environment_loader.h

    # API & Backend
    api/renderer_api.cpp
    api/buffer.cpp
    api/vertex_array.cpp
    api/framebuffer.cpp
    api/framebuffer.h
    api/model_data.h
    api/raylib/raylib_renderer_api.cpp
    api/raylib/raylib_buffer.cpp
    api/raylib/raylib_vertex_array.cpp
    api/raylib/raylib_framebuffer.cpp
)

target_include_directories(engine_graphics PUBLIC
    ${ENGINE_INCLUDE_DIRS}
    ${assimp_SOURCE_DIR}/include
    ${assimp_BINARY_DIR}/include
    ${yaml-cpp_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/imgui/backends
    ${CMAKE_SOURCE_DIR}/include/entt/src
    ${imguizmo_SOURCE_DIR}
)

# Graphics module depends on Core, assimp, raylib, imgui
target_link_libraries(engine_graphics PUBLIC 
    raylib 
    imguilib 
    assimp::assimp 
    yaml-cpp
    EnTT
)

if(MSVC)
    target_compile_options(engine_graphics PRIVATE /bigobj)
elseif(MINGW)
    target_compile_options(engine_graphics PRIVATE -Wa,-mbig-obj)
endif()

if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(engine_graphics)
endif()

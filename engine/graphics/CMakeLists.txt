add_library(engine_graphics OBJECT
    renderer.cpp
    ui_renderer.cpp
    asset_manager.cpp
    shader_asset.cpp
    shader_library.cpp
    scene_renderer.cpp
    renderer2d.cpp
    model_asset.cpp
    mesh_importer.cpp
    texture_importer.cpp
    shader_importer.cpp
    font_importer.cpp
    environment_importer.cpp

    renderer.h
    scene_renderer.h
    ui_renderer.h
    asset.h
    asset_manager.h
    shader_asset.h
    shader_library.h
    model_asset.h
    texture_asset.h
    font_asset.h
    environment.h
    material.h
    mesh_importer.h
    texture_importer.h
    shader_importer.h
    font_importer.h
    environment_importer.h
    asset_importer.h
    frustum.h
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

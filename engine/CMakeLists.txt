
# Common include directories for all engine targets
set(ENGINE_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Build entire engine as one SHARED library
add_library(engine SHARED
    # Core
    core/layer_stack.cpp
    core/profiler.cpp
    core/uuid.cpp
    core/window.cpp
    core/input.cpp
    core/imgui_layer.cpp
    core/thread_pool.cpp
    core/module_loader.cpp
    core/application.cpp
    
    # Graphics
    graphics/render.cpp
    graphics/render_command.cpp
    graphics/ui_renderer.cpp
    graphics/asset_manager.cpp
    graphics/shader_asset.cpp
    graphics/scene_renderer.cpp
    graphics/model_asset.cpp
    graphics/texture_asset.cpp
    graphics/font_asset.cpp
    graphics/environment.cpp
    
    # Audio
    audio/audio.cpp
    audio/sound_asset.cpp
    
    # Physics
    physics/physics.cpp
    physics/dynamics.cpp
    physics/narrow_phase.cpp
    physics/scene_trace.cpp
    physics/bvh/bvh.cpp
    
    # Scene
    scene/scene.cpp
    scene/entity.cpp
    scene/project.cpp
    scene/project_serializer.cpp
    scene/scene_serializer.cpp
    scene/component_serializer.cpp
    scene/component_serializer_ui.cpp
    scene/prefab_serializer.cpp
    scene/scene_scripting.cpp
    scene/scene_camera.cpp
    
    # All headers (for IDE support)
    core/application.h
    core/imgui_layer.h
    core/base.h
    core/assert.h
    core/platform_detection.h
    core/thread_pool.h
    core/entry_point.h
    core/events.h
    core/input.h
    core/layer.h
    core/layer_stack.h
    core/log.h
    core/profiler.h
    core/timestep.h
    core/uuid.h
    core/window.h
    core/yaml.h
    core/module_loader.h
    
    graphics/render.h
    graphics/render_command.h
    graphics/scene_renderer.h
    graphics/ui_renderer.h
    graphics/asset.h
    graphics/asset_manager.h
    graphics/shader_asset.h
    graphics/model_asset.h
    graphics/texture_asset.h
    graphics/font_asset.h
    graphics/environment.h
    graphics/material.h
    
    audio/audio.h
    audio/sound_asset.h
    
    physics/physics.h
    physics/dynamics.h
    physics/narrow_phase.h
    physics/scene_trace.h
    physics/bvh/bvh.h
    physics/bvh/bvh_node.h
    physics/collision/collision.h
    physics/collision/collision_triangle.h
    
    scene/scene.h
    scene/scene_events.h
    scene/project.h
    scene/project_serializer.h
    scene/scene_serializer.h
    scene/component_serializer.h
    scene/prefab_serializer.h
    scene/scene_scripting.h
    scene/script_registry.h
    scene/scriptable_entity.h
    scene/scene_camera.h
    scene/components.h
    scene/components/animation_component.h
    scene/components/audio_component.h
    scene/components/billboard_component.h
    scene/components/camera_component.h
    scene/components/control_component.h
    scene/components/game_components.h
    scene/components/hierarchy_component.h
    scene/components/id_component.h
    scene/components/lighting_component.h
    scene/components/mesh_component.h
    scene/components/navigation_component.h
    scene/components/physics_component.h
    scene/components/scripting_components.h
    scene/components/shader_component.h
    scene/components/tag_component.h
    scene/components/transform_component.h
)

target_include_directories(engine PUBLIC
    ${ENGINE_INCLUDE_DIRS}
    ${cgltf_SOURCE_DIR}
    ${tinyobjloader_SOURCE_DIR}
INTERFACE
    ${yaml-cpp_SOURCE_DIR}/include
)

target_link_libraries(engine 
PUBLIC 
    raylib 
    glm 
    EnTT
    imguilib
PRIVATE
    yaml-cpp
)

# Compiler flags
if(MSVC)
    target_compile_options(engine PRIVATE /bigobj)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(engine PRIVATE -Wa,-mbig-obj)
endif()

if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(engine)
endif()

install(TARGETS engine
    EXPORT ChainedEngineTargets
    ARCHIVE DESTINATION lib COMPONENT Runtime
    LIBRARY DESTINATION lib COMPONENT Runtime
    RUNTIME DESTINATION bin COMPONENT Runtime
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    DESTINATION engine
    USE_SOURCE_PERMISSIONS
    COMPONENT Runtime
)

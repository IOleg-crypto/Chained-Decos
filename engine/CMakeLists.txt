set(ENGINE_SOURCES
    # Core
    core/application.cpp
    core/window.cpp
    core/window.h
    core/profiler.cpp
    core/profiler.h
    core/uuid.cpp
    core/uuid.h
    core/input.cpp
    core/input.h
    core/layer_stack.cpp
    core/layer_stack.h
    core/process_utils.cpp
    core/process_utils.h
    core/base.h
    core/events.h
    core/entry_point.h
    core/math_types.h
    core/task_system.cpp
    core/task_system.h

    # Scene
    scene/scene.cpp
    scene/scene.h
    scene/entity.h
    scene/components.h
    scene/scene_serializer.cpp
    scene/scene_serializer.h
    scene/project.cpp
    scene/project.h
    scene/project_serializer.cpp
    scene/project_serializer.h
    scene/script_registry.h
    scene/scriptable_entity.h
    scene/yaml_utils.h

    # Components (Subfolders)
    scene/components/audio_component.h
    scene/components/camera_component.h
    scene/components/game_components.h
    scene/components/hierarchy_component.h
    scene/components/id_component.h
    scene/components/lighting_component.h
    scene/components/mesh_component.h
    scene/components/physics_component.h
    scene/components/scripting_components.h
    scene/components/tag_component.h
    scene/components/transform_component.h
    scene/components/widget_component.h
    scene/components/animation_component.h

    # Renderer
    renderer/visuals.cpp
    renderer/visuals.h
    renderer/api_context.cpp
    renderer/api_context.h
    renderer/draw_command.cpp
    renderer/draw_command.h
    renderer/scene_pipeline.cpp
    renderer/scene_pipeline.h
    renderer/render.h
    renderer/render_types.h

    renderer/asset_manager.cpp
    renderer/asset_manager.h
    renderer/asset_archive.cpp
    renderer/asset_archive.h
    renderer/resource_provider.cpp
    renderer/resource_provider.h

    renderer/model_asset.cpp
    renderer/model_asset.h
    renderer/texture_asset.cpp
    renderer/texture_asset.h
    renderer/shader_asset.cpp
    renderer/shader_asset.h
    renderer/environment.cpp
    renderer/environment.h
    renderer/font_asset.cpp
    renderer/font_asset.h
    renderer/asset.h
    renderer/material.h


    # Physics
    physics/physics.cpp
    physics/physics.h
    physics/dynamics.cpp
    physics/dynamics.h
    physics/narrow_phase.cpp
    physics/narrow_phase.h
    physics/scene_trace.cpp
    physics/scene_trace.h

    physics/collision/collision.h
    physics/collision/collision_triangle.h
    physics/bvh/bvh.cpp
    physics/bvh/bvh.h
    physics/bvh/bvh_node.h

    # Audio
    audio/sound_asset.cpp
    audio/sound_asset.h

    # UI
    ui/canvas_renderer.cpp
    ui/canvas_renderer.h
    ui/imgui_raylib_ui.h
)

add_library(engine STATIC ${ENGINE_SOURCES})
target_link_libraries(engine PUBLIC yaml-cpp)

target_link_libraries(engine PUBLIC raylib imguilib EnTT)
target_include_directories(engine PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
target_include_directories(engine PRIVATE ${CMAKE_SOURCE_DIR}/include/json/single_include)

# Copy engine resources to the build directory
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources" DESTINATION "${CMAKE_BINARY_DIR}/engine")

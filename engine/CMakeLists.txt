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
    core/main_thread_queue.cpp
    core/main_thread_queue.h
    core/async_utils.h
    core/base.h
    core/events.h
    core/entry_point.h
    core/math_types.h

    # Scene
    scene/scene.cpp
    scene/scene.h
    scene/scene_scripting.cpp
    scene/scene_scripting.h
    scene/scene_animator.cpp
    scene/scene_animator.h
    scene/scene_audio.cpp
    scene/scene_audio.h
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
    scene/components/animation_component.h
    scene/components/hierarchy_component.h
    scene/components/scripting_components.h
    scene/components/ui_component.h

    # Renderer
    renderer/render.cpp
    renderer/render.h
    renderer/render_types.h
    renderer/scene_render.cpp
    renderer/scene_render.h
    renderer/asset_manager.cpp
    renderer/asset_manager.h
    renderer/model_asset.cpp
    renderer/model_asset.h
    renderer/texture_asset.cpp
    renderer/texture_asset.h
    renderer/shader_asset.cpp
    renderer/shader_asset.h
    renderer/environment.cpp
    renderer/environment.h
    renderer/asset.h
    renderer/material.h
    renderer/render_state.h

    # Physics
    physics/physics.cpp
    physics/physics.h
    physics/collision/collision.h
    physics/collision/collision_triangle.h
    physics/bvh/bvh.cpp
    physics/bvh/bvh.h
    physics/bvh/bvh_node.h

    # Audio
    audio/sound_asset.cpp
    audio/sound_asset.h
)

add_library(engine STATIC ${ENGINE_SOURCES})
target_link_libraries(engine PUBLIC yaml-cpp)

target_link_libraries(engine PUBLIC raylib imguilib EnTT)
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(engine PUBLIC ${CMAKE_SOURCE_DIR}/include/json/single_include)

# Copy engine resources to the build directory
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources" DESTINATION "${CMAKE_BINARY_DIR}/engine")

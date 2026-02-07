
# Common include directories for all engine targets
set(ENGINE_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)


add_library(engine_core STATIC
    # Sources
    core/layer_stack.cpp
    core/profiler.cpp
    core/uuid.cpp
    core/window.cpp
    core/input.cpp
    core/imgui_layer.cpp
    core/thread_pool.cpp
    
    # Headers
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
)

target_include_directories(engine_core PUBLIC ${ENGINE_INCLUDE_DIRS})
target_link_libraries(engine_core PUBLIC raylib yaml-cpp imguilib glm EnTT)


add_library(engine_graphics STATIC
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
)

target_include_directories(engine_graphics PUBLIC 
    ${ENGINE_INCLUDE_DIRS}
    ${cgltf_SOURCE_DIR}
    ${tinyobjloader_SOURCE_DIR}
)
target_link_libraries(engine_graphics PUBLIC engine_core raylib)


add_library(engine_audio STATIC
    audio/audio.cpp
    audio/sound_asset.cpp
    audio/audio.h
    audio/sound_asset.h
)

target_include_directories(engine_audio PUBLIC ${ENGINE_INCLUDE_DIRS})
target_link_libraries(engine_audio PUBLIC engine_core)


add_library(engine_physics STATIC
    # Sources
    physics/physics.cpp
    physics/dynamics.cpp
    physics/narrow_phase.cpp
    physics/scene_trace.cpp
    physics/bvh/bvh.cpp
    
    # Headers
    physics/physics.h
    physics/dynamics.h
    physics/narrow_phase.h
    physics/scene_trace.h
    physics/bvh/bvh.h
    physics/bvh/bvh_node.h
    physics/collision/collision.h
    physics/collision/collision_triangle.h
)

target_include_directories(engine_physics PUBLIC ${ENGINE_INCLUDE_DIRS})
target_link_libraries(engine_physics PUBLIC engine_core)

add_library(engine_scene STATIC
    # Sources
    scene/scene.cpp
    scene/entity.cpp
    scene/project.cpp
    scene/project_serializer.cpp
    scene/scene_serializer.cpp
    scene/component_serializer.cpp
    scene/component_serializer_ui.cpp
    scene/serialization/graphics_serialization.cpp
    scene/serialization/physics_serialization.cpp
    scene/serialization/audio_gameplay_serialization.cpp
    scene/prefab_serializer.cpp
    scene/scene_scripting.cpp
    scene/scene_camera.cpp
    
    # Headers
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
    
    # Component Headers
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

target_include_directories(engine_scene PUBLIC ${ENGINE_INCLUDE_DIRS})
target_link_libraries(engine_scene PUBLIC 
    engine_core
    engine_graphics
    engine_physics
    engine_audio
)

add_library(engine_application STATIC
    core/application.cpp
)
target_include_directories(engine_application PUBLIC ${ENGINE_INCLUDE_DIRS})
target_link_libraries(engine_application PUBLIC engine_scene)


add_library(engine INTERFACE)
target_link_libraries(engine INTERFACE
    engine_core
    engine_graphics
    engine_audio
    engine_physics
    engine_scene
    engine_application
)

set(ENGINE_TARGETS engine_core engine_graphics engine_audio engine_physics engine_scene engine_application)

foreach(target ${ENGINE_TARGETS})
    if(MSVC)
        target_compile_options(${target} PRIVATE /bigobj)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -W)
    endif()
    if(COMMAND apply_engine_optimizations)
        apply_engine_optimizations(${target})
    endif()
endforeach()

install(TARGETS ${ENGINE_TARGETS}
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

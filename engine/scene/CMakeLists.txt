# 1. Core library (No UI / Graphics dependencies)
add_library(engine_scene_core STATIC
    entity.cpp entity.h
    scene.cpp scene.h
    scene_context.h scene_events.h scene_settings.h scene_state.h
    camera.cpp camera.h
    components.h
    components/animation_component.h
    components/audio_component.h
    components/camera_component.h
    components/component_utils.h
    components/hierarchy_component.h
    components/id_component.h
    components/light_component.h
    components/model_component.h
    components/physics_component.h
    components/player_component.h
    components/primitive_component.h
    components/scene_transition_component.h
    components/scripting_components.h
    components/shader_component.h
    components/spawn_component.h
    components/sprite_component.h
    components/tag_component.h
    components/transform_component.h
)
target_include_directories(engine_scene_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(engine_scene_core PUBLIC engine_reflection engine_assets engine_external_entt glm)
if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(engine_scene_core)
endif()
set_target_properties(engine_scene_core PROPERTIES UNITY_BUILD OFF)

# 2. Main scene library (Serialization, Registry, UI Components)
chained_add_engine_module(engine_scene
    EXCLUDE_FILES
        entity.cpp entity.h
        scene.cpp scene.h
        scene_context.h scene_events.h scene_settings.h scene_state.h
        camera.cpp camera.h
        components.h
        animation_component.h
        audio_component.h
        camera_component.h
        component_utils.h
        hierarchy_component.h
        id_component.h
        light_component.h
        model_component.h
        physics_component.h
        player_component.h
        primitive_component.h
        scene_transition_component.h
        scripting_components.h
        shader_component.h
        spawn_component.h
        sprite_component.h
        tag_component.h
        transform_component.h
    DEPENDS
        engine_scene_core
        engine_reflection
        engine_core
        engine_graphics
        engine_project
        engine_physics
        engine_audio
        engine_scripting
        engine_external_entt
        glm
        engine_external_imgui
)
set_target_properties(engine_scene PROPERTIES UNITY_BUILD OFF)

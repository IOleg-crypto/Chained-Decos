# 1. Core library (No UI / Graphics dependencies)
add_library(engine_scene_core STATIC
    entity.cpp entity.h
    scene.h
    scene_events.h scene_settings.h scene_state.h
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
if(TARGET engine_pch)
    target_link_libraries(engine_scene_core PRIVATE engine_pch)
endif()
set_target_properties(engine_scene_core PROPERTIES UNITY_BUILD OFF)

# 2. Main scene library (Serialization, Registry, UI Components)
add_library(engine_scene STATIC
    component_registry.cpp
    component_registry_core.cpp
    component_registry_gameplay.cpp
    component_registry_physics.cpp
    component_registry_rendering.cpp
    component_registry_scripting.cpp
    component_registry_ui.cpp
    component_ui_control.cpp
    component_serializer.cpp
    hierarchy_serializer.cpp
    prefab_serializer.cpp
    scene.cpp
    scene_serializer.cpp
    systems/hierarchy_system.h
    systems/hierarchy_system.cpp
    systems/animation_system.h
    systems/animation_system.cpp
    systems/audio_system.h
    systems/audio_system.cpp
    systems/physics_body_system.h
    systems/physics_body_system.cpp
    systems/asset_resolution_system.h
    systems/asset_resolution_system.cpp
    systems/scene_transition_system.h
    systems/scene_transition_system.cpp
    systems/network_system.h
    systems/network_system.cpp
    systems/nametag_system.h
    systems/nametag_system.cpp
    systems/system_manager.h
    systems/system_manager.cpp
    components/scripting_components.cpp
)

target_include_directories(engine_scene PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(engine_scene PUBLIC
    engine_scene_core
    engine_reflection
    engine_core
    engine_graphics
    engine_project
    engine_physics
    engine_audio
    engine_scripting
    engine_network
    engine_external_entt
    glm
    engine_external_imgui
)

if(TARGET engine_pch)
    target_link_libraries(engine_scene PRIVATE engine_pch)
endif()

set_target_properties(engine_scene PROPERTIES UNITY_BUILD OFF)

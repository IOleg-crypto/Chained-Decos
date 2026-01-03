cmake_minimum_required(VERSION 3.20)

# ECS library
set(ECS_SOURCES
    ecs_registry.h
    components.h
    systems.h
    components/transform_component.h
    components/velocity_component.h
    components/player_component.h
    components/render_component.h
    components/physics_data.h
    components/audio_component.h
    components/camera_component.h
    components/utility_components.h
    components/ui_components.h
    systems/ui_render_system.h
    systems/ui_render_system.cpp
    systems/ui_system.h
    systems/ui_system.cpp
    systems/physics_system.h
    systems/physics_system.cpp
    systems/skybox_system.h
    systems/skybox_system.cpp
    systems/render_system.h
    systems/render_system.cpp
    systems/lifetime_system.h
    systems/lifetime_system.cpp
    systems/player_system.h
    systems/player_system.cpp
    systems/entity_collision_system.h
    systems/entity_collision_system.cpp
)

add_library(ecs STATIC ${ECS_SOURCES})

target_include_directories(ecs PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include/json/include
)

target_link_libraries(ecs PUBLIC
    EnTT::EnTT
    raylib
    font_resources
    texture_resources
    imguilib
    scene_core
    collision_core
)

set_target_properties(ecs PROPERTIES POSITION_INDEPENDENT_CODE ON)



cmake_minimum_required(VERSION 3.20)

# ECS library
set(ECS_SOURCES
    ECSRegistry.h
    components.h
    systems.h
    components/TransformComponent.h
    components/VelocityComponent.h
    components/PlayerComponent.h
    components/RenderComponent.h
    components/PhysicsData.h
    components/AudioComponent.h
    components/CameraComponent.h
    components/UtilityComponents.h
    components/UIComponents.h
    systems/UIRenderSystem.h
    systems/UIRenderSystem.cpp
    systems/ScriptSystem.h
    systems/ScriptSystem.cpp
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
    scene_manager
)

set_target_properties(ecs PROPERTIES POSITION_INDEPENDENT_CODE ON)



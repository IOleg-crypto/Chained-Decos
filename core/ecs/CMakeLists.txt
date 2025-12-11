cmake_minimum_required(VERSION 3.20)

# ECS library
add_library(ecs STATIC
    ECSRegistry.cpp
    ECSRegistry.h
    Components.h
    Systems.h
    Components/TransformComponent.h
    Components/VelocityComponent.h
    Components/PlayerComponent.h
    Components/RenderComponent.h
    Components/PhysicsComponent.h
    Components/AudioComponent.h
    Components/CameraComponent.h
    Components/UtilityComponents.h
    Systems/MovementSystem.h
    Systems/MovementSystem.cpp
    Systems/PlayerSystem.h
    Systems/PlayerSystem.cpp
    Systems/RenderSystem.h
    Systems/RenderSystem.cpp
    Systems/CollisionSystem.h
    Systems/CollisionSystem.cpp
    Systems/LifetimeSystem.h
    Systems/LifetimeSystem.cpp
)

target_include_directories(ecs PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include/json/include
)

target_link_libraries(ecs PUBLIC
    EnTT::EnTT
    raylib
)

set_target_properties(ecs PROPERTIES POSITION_INDEPENDENT_CODE ON)

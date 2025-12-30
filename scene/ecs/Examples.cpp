#include "scene/ecs/Examples.h"
#include "scene/ecs/components/AudioComponent.h"
#include "scene/ecs/components/CameraComponent.h"
#include "scene/ecs/components/PhysicsData.h"
#include "scene/ecs/components/PlayerComponent.h"
#include "scene/ecs/components/RenderComponent.h"
#include "scene/ecs/components/TransformComponent.h"
#include "scene/ecs/components/UtilityComponents.h"
#include "scene/ecs/components/VelocityComponent.h"
#include <raymath.h>


using namespace CHEngine;

namespace CHEngine
{
namespace ECSExamples
{

entt::entity CreatePlayer(entt::registry &registry, Vector3 position, Model *model, float moveSpeed,
                          float jumpForce, float mouseSensitivity, Vector3 spawnPosition)
{
    auto player = registry.create();

    // Transform
    registry.emplace<TransformComponent>(player,
                                         position,         // position
                                         Vector3{0, 0, 0}, // rotation
                                         Vector3{1, 1, 1}  // scale
    );

    // Velocity
    registry.emplace<VelocityComponent>(player);

    // Render
    if (model)
    {
        registry.emplace<RenderComponent>(player,
                                          "player", // modelName
                                          model,    // model
                                          GRAY,     // tint
                                          true,     // visible
                                          1         // renderLayer (higher than static)
        );
    }

    // Player-specific
    auto &pc = registry.emplace<PlayerComponent>(player,
                                                 moveSpeed,       // moveSpeed
                                                 jumpForce,       // jumpForce
                                                 mouseSensitivity // mouseSensitivity
    );
    pc.spawnPosition = spawnPosition;

    // Physics
    registry.emplace<PhysicsData>(player,
                                  1.0f,  // mass
                                  -9.8f, // gravity
                                  true,  // useGravity
                                  false  // isKinematic
    );

    // Collision
    CollisionComponent collision;
    // BoundingBox: min at 0 (feet), max at 1.8 (head)
    collision.bounds = BoundingBox{Vector3{-0.4f, 0.0f, -0.4f}, Vector3{0.4f, 1.8f, 0.4f}};
    collision.collisionLayer = 1; // Player layer
    registry.emplace<CollisionComponent>(player, collision);

    // Name for debugging
    registry.emplace<NameComponent>(player, "Player");

    return player;
}

entt::entity CreateEnemy(entt::registry &registry, Vector3 position, Model *model)
{
    auto enemy = registry.create();

    registry.emplace<TransformComponent>(enemy, position);
    registry.emplace<VelocityComponent>(enemy);

    // Render
    registry.emplace<RenderComponent>(enemy,
                                      "enemy", // modelName
                                      model,   // model
                                      RED,     // tint
                                      true,    // visible
                                      0        // renderLayer
    );

    // Physics
    registry.emplace<PhysicsData>(enemy);

    // Collision
    CollisionComponent collision;
    collision.bounds = BoundingBox{Vector3{-0.5f, -0.5f, -0.5f}, Vector3{0.5f, 0.5f, 0.5f}};
    collision.collisionLayer = 2; // Enemy layer
    registry.emplace<CollisionComponent>(enemy, collision);

    registry.emplace<NameComponent>(enemy, "Enemy");

    return enemy;
}

entt::entity CreateBullet(entt::registry &registry, Vector3 position, Vector3 direction,
                          float speed)
{
    auto bullet = registry.create();

    registry.emplace<TransformComponent>(bullet, position);

    VelocityComponent velocity;
    velocity.velocity = Vector3Scale(direction, speed);
    registry.emplace<VelocityComponent>(bullet, velocity);

    // Lifetime - destroy after 5 seconds
    registry.emplace<LifetimeComponent>(bullet, 5.0f);

    // Collision
    CollisionComponent collision;
    collision.bounds = BoundingBox{Vector3{-0.1f, -0.1f, -0.1f}, Vector3{0.1f, 0.1f, 0.1f}};
    collision.isTrigger = true;   // Does not block movement
    collision.collisionLayer = 3; // Bullet layer
    registry.emplace<CollisionComponent>(bullet, collision);

    registry.emplace<TagComponent>(bullet, "Bullet");

    return bullet;
}

entt::entity CreateCamera(entt::registry &registry, Vector3 position, Vector3 target)
{
    auto cameraEntity = registry.create();

    registry.emplace<TransformComponent>(cameraEntity, position);

    CameraComponent camera;
    camera.camera.position = position;
    camera.camera.target = target;
    camera.camera.up = Vector3{0, 1, 0};
    camera.camera.fovy = 60.0f;
    camera.camera.projection = CAMERA_PERSPECTIVE;
    camera.isActive = true;
    camera.priority = 0;
    registry.emplace<CameraComponent>(cameraEntity, camera);

    registry.emplace<NameComponent>(cameraEntity, "MainCamera");

    return cameraEntity;
}

entt::entity CreateStaticObject(entt::registry &registry, Vector3 position, Model *model,
                                BoundingBox bounds)
{
    auto obj = registry.create();

    registry.emplace<TransformComponent>(obj, position);

    registry.emplace<RenderComponent>(obj, "static", model, WHITE, true, 0);

    // Kinematic physics - does not move
    PhysicsData physics;
    physics.isKinematic = true;
    physics.useGravity = false;
    registry.emplace<PhysicsData>(obj, physics);

    CollisionComponent collision;
    collision.bounds = bounds;
    collision.collisionLayer = 0; // Static layer
    registry.emplace<CollisionComponent>(obj, collision);

    registry.emplace<TagComponent>(obj, "Static");

    return obj;
}

} // namespace ECSExamples
} // namespace CHEngine

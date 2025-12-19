#include "core/ecs/Examples.h"
#include "core/ecs/ECSRegistry.h"
#include "core/ecs/components.h"
#include <raymath.h>


namespace ECSExamples
{

entt::entity CreatePlayer(Vector3 position, Model *model, float moveSpeed, float jumpForce,
                          float mouseSensitivity)
{
    auto player = REGISTRY.create();

    // Transform
    REGISTRY.emplace<TransformComponent>(player,
                                         position,         // position
                                         Vector3{0, 0, 0}, // rotation
                                         Vector3{1, 1, 1}  // scale
    );

    // Velocity
    REGISTRY.emplace<VelocityComponent>(player);

    // Render
    if (model)
    {
        REGISTRY.emplace<RenderComponent>(player,
                                          "player", // modelName
                                          model,    // model
                                          GRAY,     // tint
                                          true,     // visible
                                          1         // renderLayer (higher than static)
        );
    }

    // Player-specific
    REGISTRY.emplace<PlayerComponent>(player,
                                      moveSpeed,       // moveSpeed
                                      jumpForce,       // jumpForce
                                      mouseSensitivity // mouseSensitivity
    );

    // Physics
    REGISTRY.emplace<PhysicsData>(player,
                                  1.0f,  // mass
                                  -9.8f, // gravity
                                  true,  // useGravity
                                  false  // isKinematic
    );

    // Collision
    CollisionComponent collision;
    collision.bounds =
        BoundingBox{Vector3{-0.4f, 0.0f, -0.4f}, Vector3{0.4f, 1.8f, 0.4f}}; // 0.8w x 1.8h
    collision.collisionLayer = 1;                                            // Player layer
    REGISTRY.emplace<CollisionComponent>(player, collision);

    // Name for debugging
    REGISTRY.emplace<NameComponent>(player, "Player");

    return player;
}

entt::entity CreateEnemy(Vector3 position, Model *model)
{
    auto enemy = REGISTRY.create();

    REGISTRY.emplace<TransformComponent>(enemy, position);
    REGISTRY.emplace<VelocityComponent>(enemy);

    // Render
    REGISTRY.emplace<RenderComponent>(enemy,
                                      "enemy", // modelName
                                      model,   // model
                                      RED,     // tint
                                      true,    // visible
                                      0        // renderLayer
    );

    // Physics
    REGISTRY.emplace<PhysicsData>(enemy);

    // Collision
    CollisionComponent collision;
    collision.bounds = BoundingBox{Vector3{-0.5f, -0.5f, -0.5f}, Vector3{0.5f, 0.5f, 0.5f}};
    collision.collisionLayer = 2; // Enemy layer
    REGISTRY.emplace<CollisionComponent>(enemy, collision);

    REGISTRY.emplace<NameComponent>(enemy, "Enemy");

    return enemy;
}

entt::entity CreateBullet(Vector3 position, Vector3 direction, float speed)
{
    auto bullet = REGISTRY.create();

    REGISTRY.emplace<TransformComponent>(bullet, position);

    VelocityComponent velocity;
    velocity.velocity = Vector3Scale(direction, speed);
    REGISTRY.emplace<VelocityComponent>(bullet, velocity);

    // Lifetime - destroy after 5 seconds
    REGISTRY.emplace<LifetimeComponent>(bullet, 5.0f);

    // Collision
    CollisionComponent collision;
    collision.bounds = BoundingBox{Vector3{-0.1f, -0.1f, -0.1f}, Vector3{0.1f, 0.1f, 0.1f}};
    collision.isTrigger = true;   // Does not block movement
    collision.collisionLayer = 3; // Bullet layer
    REGISTRY.emplace<CollisionComponent>(bullet, collision);

    REGISTRY.emplace<TagComponent>(bullet, "Bullet");

    return bullet;
}

entt::entity CreateCamera(Vector3 position, Vector3 target)
{
    auto cameraEntity = REGISTRY.create();

    REGISTRY.emplace<TransformComponent>(cameraEntity, position);

    CameraComponent camera;
    camera.camera.position = position;
    camera.camera.target = target;
    camera.camera.up = Vector3{0, 1, 0};
    camera.camera.fovy = 60.0f;
    camera.camera.projection = CAMERA_PERSPECTIVE;
    camera.isActive = true;
    camera.priority = 0;
    REGISTRY.emplace<CameraComponent>(cameraEntity, camera);

    REGISTRY.emplace<NameComponent>(cameraEntity, "MainCamera");

    return cameraEntity;
}

entt::entity CreateStaticObject(Vector3 position, Model *model, BoundingBox bounds)
{
    auto obj = REGISTRY.create();

    REGISTRY.emplace<TransformComponent>(obj, position);

    REGISTRY.emplace<RenderComponent>(obj, "static", model, WHITE, true, 0);

    // Kinematic physics - does not move
    PhysicsData physics;
    physics.isKinematic = true;
    physics.useGravity = false;
    REGISTRY.emplace<PhysicsData>(obj, physics);

    CollisionComponent collision;
    collision.bounds = bounds;
    collision.collisionLayer = 0; // Static layer
    REGISTRY.emplace<CollisionComponent>(obj, collision);

    REGISTRY.emplace<TagComponent>(obj, "Static");

    return obj;
}

} // namespace ECSExamples





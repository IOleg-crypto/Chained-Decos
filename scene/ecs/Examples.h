#ifndef ECS_EXAMPLES_H
#define ECS_EXAMPLES_H

#include <entt/entt.hpp>
#include <raylib.h>

// Examples of creating different types of entities

namespace ECSExamples
{

// Create player
entt::entity CreatePlayer(Vector3 position, Model *model, float moveSpeed = 8.0f,
                          float jumpForce = 12.0f, float mouseSensitivity = 0.15f,
                          Vector3 spawnPosition = {0, 0, 0});

// Create enemy
entt::entity CreateEnemy(Vector3 position, Model *model);

// Create bullet
entt::entity CreateBullet(Vector3 position, Vector3 direction, float speed);

// Create camera
entt::entity CreateCamera(Vector3 position, Vector3 target);

// Create static object (wall, floor)
entt::entity CreateStaticObject(Vector3 position, Model *model, BoundingBox bounds);

} // namespace ECSExamples

#endif // ECS_EXAMPLES_H

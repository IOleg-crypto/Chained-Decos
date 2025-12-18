#include "MovementSystem.h"
#include "components/physics/collision/Core/CollisionManager.h"
#include "core/ecs/Components/PhysicsData.h"
#include "core/ecs/Components/PlayerComponent.h"
#include "core/ecs/Components/TransformComponent.h"
#include "core/ecs/Components/VelocityComponent.h"
#include "core/ecs/ECSRegistry.h"
#include "core/engine/Engine.h"
#include <raymath.h>

namespace MovementSystem
{

static void ApplyGravity(float dt)
{
    auto view = REGISTRY.view<VelocityComponent, PhysicsData, PlayerComponent>();

    for (auto &&[entity, velocity, physics, player] : view.each())
    {
        if (physics.useGravity && !physics.isKinematic)
        {
            // Only apply gravity if not grounded
            if (!player.isGrounded)
            {
                velocity.acceleration.y = physics.gravity;
            }
            else
            {
                velocity.acceleration.y = 0.0f;
                // velocity.velocity.y = 0.0f; // Handled in collision check
            }
        }
    }
}

static void CheckCollisionsAndApply(float dt)
{
    auto view = REGISTRY.view<TransformComponent, VelocityComponent, PlayerComponent, PhysicsData,
                              CollisionComponent>();
    auto collisionManager = Engine::Instance().GetService<CollisionManager>();

    for (auto &&[entity, transform, velocity, player, physics, collision] : view.each())
    {
        // 1. Update Velocity
        velocity.velocity.x += velocity.acceleration.x * dt;
        velocity.velocity.y += velocity.acceleration.y * dt;
        velocity.velocity.z += velocity.acceleration.z * dt;

        Vector3 oldPos = transform.position;
        Vector3 proposedPos = oldPos;

        // 2. Horizontal Movement (X/Z)
        proposedPos.x += velocity.velocity.x * dt;
        proposedPos.z += velocity.velocity.z * dt;

        bool collisionX = false;
        bool collisionZ = false;

        if (collisionManager)
        {
            // Use bounds from CollisionComponent
            Vector3 center = proposedPos;
            center.x += (collision.bounds.max.x + collision.bounds.min.x) * 0.5f;
            center.y += (collision.bounds.max.y + collision.bounds.min.y) * 0.5f;
            center.z += (collision.bounds.max.z + collision.bounds.min.z) * 0.5f;

            Vector3 halfSize;
            halfSize.x = (collision.bounds.max.x - collision.bounds.min.x) * 0.5f;
            halfSize.y = (collision.bounds.max.y - collision.bounds.min.y) * 0.5f;
            halfSize.z = (collision.bounds.max.z - collision.bounds.min.z) * 0.5f;

            Collision playerCol(center, halfSize);
            Vector3 response = {0};

            if (collisionManager->CheckCollision(playerCol, response))
            {
                // Push back
                proposedPos = Vector3Add(proposedPos, response);

                // Slide: Project velocity onto sliding plane
                // Normal is roughly normalized response
                // If magnitude is small, response might be zero, careful
                float responseLen = Vector3Length(response);
                if (responseLen > 0.001f)
                {
                    Vector3 normal = Vector3Scale(response, 1.0f / responseLen);

                    // v_new = v - (v . n) * n
                    float dot = Vector3DotProduct(velocity.velocity, normal);

                    // Only remove velocity component opposing the normal
                    if (dot < 0)
                    {
                        Vector3 remove = Vector3Scale(normal, dot);
                        velocity.velocity = Vector3Subtract(velocity.velocity, remove);
                    }
                }
            }
        }

        // 3. Vertical Movement (Y) and Ground Check
        proposedPos.y += velocity.velocity.y * dt;

        bool onGround = false;

        if (collisionManager)
        {
            // Raycast down to find ground
            float hitDist = 0.0f;
            Vector3 hitPoint;
            Vector3 hitNormal;
            // Raycast from slightly up, down
            Vector3 rayOrigin = proposedPos;
            rayOrigin.y += 1.0f;

            // Raycast length: 1.0 (to feet) + 0.1 (tolerance)
            if (collisionManager->RaycastDown(rayOrigin, 1.2f, hitDist, hitPoint, hitNormal))
            {
                // TraceLog(LOG_INFO, "Ground Hit: %.2f", hitDist);
                // Determine if we are on ground based on detailed hit
                // If distance implies we are close to ground
                // origin y (feet + 1) - hit y = distance
                // feet y = origin y - 1
                // dist = (feet + 1) - hitY
                // hitY = feet + 1 - dist

                float distFromFeet = hitDist - 1.0f;

                // If we are falling and hit ground (or slightly embedded)
                if (velocity.velocity.y <= 0 && distFromFeet <= 0.1f)
                {
                    onGround = true;
                    proposedPos.y = hitPoint.y; // Snap to ground
                    velocity.velocity.y = 0;
                }
            }

            // Also check body collision for ceiling/floor if raycast missed
            // ... (Skipped for brevity/simplicity, Raycast is main ground check)
        }

        player.isGrounded = onGround;
        transform.position = proposedPos;
    }
}

static void ApplyDrag(float dt)
{
    auto view = REGISTRY.view<VelocityComponent>();

    for (auto &&[entity, velocity] : view.each())
    {
        float drag = 1.0f - (velocity.drag * dt);
        if (drag < 0)
            drag = 0;
        velocity.velocity.x *= drag;
        velocity.velocity.z *= drag;
    }
}

void Update(float deltaTime)
{
    ApplyGravity(deltaTime);
    CheckCollisionsAndApply(deltaTime);
    ApplyDrag(deltaTime);
}

} // namespace MovementSystem

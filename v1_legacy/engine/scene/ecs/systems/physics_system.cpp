#include "physics_system.h"
#include "engine/physics/collision/core/collision_manager.h"
#include "engine/physics/collision/core/physics.h"
#include "engine/scene/ecs/components/physics_data.h"
#include "engine/scene/ecs/components/player_component.h"
#include "engine/scene/ecs/components/transform_component.h"
#include "engine/scene/ecs/components/velocity_component.h"
#include <raylib.h>
#include <raymath.h>

namespace CHEngine
{
void PhysicsSystem::Update(entt::registry &registry, float deltaTime)
{
    // 1. Update Physics Dynamics
    auto view = registry.view<TransformComponent, VelocityComponent>();

    for (auto entity : view)
    {
        auto &transform = view.get<TransformComponent>(entity);
        auto &velocity = view.get<VelocityComponent>(entity);

        bool isGrounded = false;
        bool hasPhysicsData = registry.all_of<PhysicsData>(entity);
        bool hasPlayer = registry.all_of<PlayerComponent>(entity);

        if (hasPlayer)
        {
            isGrounded = registry.get<PlayerComponent>(entity).isGrounded;
        }

        // Apply Acceleration (Gravity)
        if (hasPhysicsData)
        {
            auto &physics = registry.get<PhysicsData>(entity);
            if (!physics.isKinematic)
            {
                if (physics.useGravity && !isGrounded)
                {
                    velocity.acceleration.y = physics.gravity;
                }
                else
                {
                    velocity.acceleration.y = 0;
                }
            }
        }

        // Integrate Velocity
        velocity.velocity =
            Vector3Add(velocity.velocity, Vector3Scale(velocity.acceleration, deltaTime));
        Vector3 proposedPos =
            Vector3Add(transform.position, Vector3Scale(velocity.velocity, deltaTime));

        // 2. World Collision Response
        if (registry.all_of<CollisionComponent>(entity))
        {
            auto &collision = registry.get<CollisionComponent>(entity);

            // Sphere/AABB vs World
            Vector3 center = proposedPos;
            center.x += (collision.bounds.max.x + collision.bounds.min.x) * 0.5f;
            center.y += (collision.bounds.max.y + collision.bounds.min.y) * 0.5f;
            center.z += (collision.bounds.max.z + collision.bounds.min.z) * 0.5f;

            Vector3 halfSize =
                Vector3Scale(Vector3Subtract(collision.bounds.max, collision.bounds.min), 0.5f);
            CHEngine::Collision entityCol(center, halfSize);
            Vector3 response = {0};

            if (CHEngine::Physics::CheckCollision(entityCol, response))
            {
                proposedPos = Vector3Add(proposedPos, response);
                float resLen = Vector3Length(response);
                if (resLen > 0.001f)
                {
                    Vector3 normal = Vector3Scale(response, 1.0f / resLen);
                    float dot = Vector3DotProduct(velocity.velocity, normal);
                    if (dot < 0)
                    {
                        velocity.velocity =
                            Vector3Subtract(velocity.velocity, Vector3Scale(normal, dot));
                    }
                }
            }

            // Ground Check (Raycast)
            Vector3 rayOrigin = proposedPos;
            rayOrigin.y += 1.0f;
            float hitDist = 0;
            Vector3 hp, hn;

            bool hit = Physics::RaycastDown(rayOrigin, 1.2f, hitDist, hp, hn);

            if (hasPlayer)
            {
                auto &player = registry.get<PlayerComponent>(entity);
                if (hit && velocity.velocity.y <= 0 && (hitDist - 1.0f) <= 0.1f)
                {
                    player.isGrounded = true;
                    proposedPos.y = hp.y;
                    velocity.velocity.y = 0;
                }
                else
                {
                    player.isGrounded = false;
                }
            }
        }

        transform.position = proposedPos;

        // 3. Apply Drag
        float dragFactor = 1.0f - (velocity.drag * deltaTime);
        if (dragFactor < 0)
            dragFactor = 0;
        velocity.velocity.x *= dragFactor;
        velocity.velocity.z *= dragFactor;

        // 4. Sync with old CollisionManager if needed (for legacy debug/picking)
        if (registry.all_of<CollisionComponent>(entity))
        {
            CollisionManager::UpdateEntityCollider(static_cast<uint32_t>(entity),
                                                   transform.position);
        }
    }
}
} // namespace CHEngine

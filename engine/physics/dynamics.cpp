#include "dynamics.h"
#include "engine/core/log.h"
#include "engine/scene/components.h"
#include "engine/project/project.h"
#include "engine/scene/scene.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Chained
{
void Dynamics::Update(::entt::registry& registry, const std::vector<entt::entity>& entities, float deltaTime, float gravity)
{
    for (auto entity : entities)
    {
        if (!registry.all_of<TransformComponent, RigidBodyComponent>(entity))
        {
            continue;
        }

        ApplyGravity(registry, entity, gravity, deltaTime);
        IntegrateVelocity(registry, entity, deltaTime);
    }
}

void Dynamics::ApplyGravity(entt::registry& registry, entt::entity entity, float gravity, float deltaTime)
{
    auto& rigidBody = registry.get<RigidBodyComponent>(entity);
    if (rigidBody.UseGravity && !rigidBody.IsGrounded && !rigidBody.IsKinematic)
    {
        rigidBody.Velocity.y -= gravity * deltaTime;
    }
}

void Dynamics::IntegrateVelocity(entt::registry& registry, entt::entity entity, float deltaTime)
{
    auto& entityTransform = registry.get<TransformComponent>(entity);
    auto& rigidBody = registry.get<RigidBodyComponent>(entity);

    // Kinematic bodies are moved entirely by scripts; skip physics-driven integration
    if (rigidBody.IsKinematic)
        return;

    // Dampen velocity slightly (Air resistance)
    float damping = 1.0f - (0.5f * deltaTime);
    rigidBody.Velocity *= damping;

    // Clamp absolute velocity to avoid "explosive" launches
    const float kMaxVelocity = 100.0f;
    float speed = glm::length(rigidBody.Velocity);
    if (speed > kMaxVelocity)
    {
        rigidBody.Velocity = glm::normalize(rigidBody.Velocity) * kMaxVelocity;
    }

    entityTransform.Translation += rigidBody.Velocity * deltaTime;
    entityTransform.IsDirty = true;
}

} // namespace Chained

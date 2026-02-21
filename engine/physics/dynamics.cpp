#include "dynamics.h"
#include "engine/core/log.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "raymath.h"

namespace CHEngine
{
void Dynamics::Update(Scene* scene, const std::vector<entt::entity>& entities, float deltaTime)
{
    auto& registry = scene->GetRegistry();

    float gravity = 20.0f;
    if (Project::GetActive())
    {
        gravity = Project::GetActive()->GetConfig().Physics.Gravity;
    }

    for (auto entity : entities)
    {
        if (!registry.all_of<TransformComponent, RigidBodyComponent>(entity))
        {
            continue;
        }

        ApplyGravity(registry, entity, gravity, deltaTime);
        LogDiagnostics(registry, entity);
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

    Vector3 velocityDelta = Vector3Scale(rigidBody.Velocity, deltaTime);
    entityTransform.Translation = Vector3Add(entityTransform.Translation, velocityDelta);
}

void Dynamics::LogDiagnostics(entt::registry& registry, entt::entity entity)
{
}
} // namespace CHEngine

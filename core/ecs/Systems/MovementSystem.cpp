#include "MovementSystem.h"
#include "core/ecs/Components/PhysicsComponent.h"
#include "core/ecs/Components/TransformComponent.h"
#include "core/ecs/Components/VelocityComponent.h"
#include "core/ecs/ECSRegistry.h"


namespace MovementSystem
{

static void ApplyGravity(float dt)
{
    auto view = REGISTRY.view<VelocityComponent, PhysicsComponent>();

    for (auto [entity, velocity, physics] : view.each())
    {
        if (physics.useGravity && !physics.isKinematic)
        {
            velocity.acceleration.y = physics.gravity;
        }
    }
}

static void ApplyVelocity(float dt)
{
    auto view = REGISTRY.view<TransformComponent, VelocityComponent>();

    // Cache-friendly iteration - дуже швидко!
    for (auto [entity, transform, velocity] : view.each())
    {
        // Оновити швидкість
        velocity.velocity.x += velocity.acceleration.x * dt;
        velocity.velocity.y += velocity.acceleration.y * dt;
        velocity.velocity.z += velocity.acceleration.z * dt;

        // Оновити позицію
        transform.position.x += velocity.velocity.x * dt;
        transform.position.y += velocity.velocity.y * dt;
        transform.position.z += velocity.velocity.z * dt;
    }
}

static void ApplyDrag(float dt)
{
    auto view = REGISTRY.view<VelocityComponent>();

    for (auto [entity, velocity] : view.each())
    {
        float drag = 1.0f - (velocity.drag * dt);
        velocity.velocity.x *= drag;
        velocity.velocity.z *= drag;
    }
}

void Update(float deltaTime)
{
    ApplyGravity(deltaTime);
    ApplyVelocity(deltaTime);
    ApplyDrag(deltaTime);
}

} // namespace MovementSystem

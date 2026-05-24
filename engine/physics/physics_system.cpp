#include "engine/physics/physics_system.h"
#include "engine/core/profiler.h"
#include "engine/physics/iphysics_world.h"
#include "engine/physics/physics.h"
#include "engine/scene/components/component_utils.h"
#include "engine/scene/scene.h"


namespace CHEngine
{
PhysicsSystem::PhysicsSystem()
{
}
PhysicsSystem::~PhysicsSystem()
{
}

void PhysicsSystem::OnInit()
{
    Physics::Init();
}
void PhysicsSystem::OnUpdate(Timestep ts)
{
}
void PhysicsSystem::OnShutdown()
{
    Physics::Shutdown();
}

void PhysicsSystem::InitializeBodies(Scene* scene)
{
    auto& registry = scene->GetRegistry();
    auto view = registry.view<TransformComponent, RigidBodyComponent>();
    auto world = registry.ctx().get<IPhysicsWorld*>();
    if (!world)
    {
        return;
    }

    for (auto entity : view)
    {
        auto [transform, rb] = view.get<TransformComponent, RigidBodyComponent>(entity);
        if (rb.Handle != kInvalidPhysicsBody)
        {
            continue;
        }

        PhysicsBodyDesc desc;
        desc.Position = transform.Translation;
        desc.Rotation = transform.RotationQuat;
        desc.IsKinematic = rb.IsKinematic;
        desc.Mass = rb.Mass;
        desc.UseGravity = rb.UseGravity;
        desc.InitialVelocity = rb.Velocity;

        rb.Handle = world->CreateBody(desc);
    }
}

void PhysicsSystem::Update(Scene* scene, Timestep ts, bool runtime)
{
    CH_PROFILE_FUNCTION();
    if (!scene)
    {
        return;
    }

    // Delegate to the robust static Physics utility which handles
    // fixed-timestep accumulation, Dynamics, and CollisionCore.
    Physics::Update(scene, ts, runtime);
}
} // namespace CHEngine

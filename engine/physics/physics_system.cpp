#include "physics_system.h"
#include "collision_core.h"
#include "dynamics.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/core/profiler.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/physics/physics.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"

namespace CHEngine
{

void PhysicsSystem::InitializeBodies(Scene* scene)
{
    auto& registry = scene->GetRegistry();
    auto& world = scene->GetPhysicsWorld();

    auto view = registry.view<RigidBodyComponent, TransformComponent>();
    for (auto entity : view)
    {
        auto& rb = view.get<RigidBodyComponent>(entity);
        auto& tc = view.get<TransformComponent>(entity);

        if (rb.Handle == kInvalidPhysicsBody)
        {
            PhysicsBodyDesc desc;
            desc.Position = tc.Translation;
            desc.Rotation = tc.RotationQuat;
            desc.InitialVelocity = rb.Velocity;
            desc.Mass = rb.Mass;
            desc.IsKinematic = rb.IsKinematic;
            desc.UseGravity = rb.UseGravity;

            rb.Handle = world.CreateBody(desc);
        }
    }
}

void PhysicsSystem::Update(Scene* scene, Timestep ts, bool runtime)
{
    if (!scene)
    {
        return;
    }

    if (!Physics::IsInitialized())
    {
        Physics::Init();
    }

    auto& registry = scene->GetRegistry();

    // Update scene statistics
    auto collView = registry.view<ColliderComponent>();
    ProfilerStats stats = Profiler::GetStats();
    stats.ColliderCount = (uint32_t)collView.size();
    Profiler::UpdateStats(stats);

    // 1. Auto-calculate collider bounds from models
    UpdateColliders(scene);

    if (!runtime)
    {
        return;
    }

    // 2. Fixed Timestep handling (using existing PhysicsContext structure for compatibility)
    float fixedTimestep = 1.0f / 60.0f;
    if (auto project = Project::GetActive())
    {
        float cfg = project->GetConfig().Physics.FixedTimestep;
        if (cfg > 0.0f)
        {
            fixedTimestep = cfg;
        }
    }

    auto& context = Physics::GetContext(scene);
    context.Accumulator += (float)ts;

    const float maxAccumulator = 0.2f;
    if (context.Accumulator > maxAccumulator)
    {
        context.Accumulator = maxAccumulator;
    }

    // 3. Step Simulation
    while (context.Accumulator >= fixedTimestep)
    {
        auto rbView = registry.view<TransformComponent, RigidBodyComponent>();
        std::vector<entt::entity> rbEntities;
        rbEntities.reserve(rbView.size_hint());

        for (auto it = rbView.begin(); it != rbView.end(); ++it)
        {
            rbEntities.push_back(*it);
        }

        if (!rbEntities.empty())
        {
            Dynamics::Update(registry, rbEntities, fixedTimestep);
            CollisionCore::ResolveCollisions(registry, rbEntities);
        }

        context.Accumulator -= fixedTimestep;
    }
}

void PhysicsSystem::UpdateColliders(Scene* scene)
{
    auto& registry = scene->GetRegistry();
    auto genView = registry.view<ColliderComponent, TransformComponent>();

    for (auto it = genView.begin(); it != genView.end(); ++it)
    {
        auto entity = *it;
        auto& collider = genView.get<ColliderComponent>(entity);
        auto& tc = genView.get<TransformComponent>(entity);

        // Only update if requested
        if (collider.AutoCalculate)
        {
            if (!registry.all_of<ModelComponent>(entity))
            {
                continue;
            }
            auto& model = registry.get<ModelComponent>(entity);
            auto asset = AssetManager::Get().Get<ModelAsset>(model.ModelPath);

            if (asset && asset->GetState() == AssetState::Ready)
            {
                if (collider.Type == ColliderType::Box)
                {
                    BoundingBox box = asset->GetBoundingBox();
                    collider.Size = box.Max - box.Min;
                    collider.Offset = box.Min;
                }
                else if (collider.Type == ColliderType::Sphere)
                {
                    BoundingBox box = asset->GetBoundingBox();
                    glm::vec3 sz = box.Max - box.Min;
                    collider.Radius = glm::max(sz.x, glm::max(sz.y, sz.z)) * 0.5f;
                    collider.Offset = (box.Min + box.Max) * 0.5f;
                }
                else if (collider.Type == ColliderType::Mesh)
                {
                    collider.ModelHandle = asset->GetID();
                    collider.ModelPath = model.ModelPath;

                    // Essential for Broadphase AABB check!
                    BoundingBox box = asset->GetBoundingBox();
                    collider.Size = box.Max - box.Min;
                    collider.Offset = box.Min;
                }
            }
        }
    }
}

void PhysicsSystem::SyncEngineToPhysics(Scene* scene)
{
    auto& registry = scene->GetRegistry();
    auto& world = scene->GetPhysicsWorld();

    auto view = registry.view<RigidBodyComponent, TransformComponent>();
    for (auto entity : view)
    {
        auto& rb = view.get<RigidBodyComponent>(entity);
        auto& tc = view.get<TransformComponent>(entity);

        if (rb.Handle != kInvalidPhysicsBody)
        {
            // For now just force sync everything or check dirty flag
            world.SetTransform(rb.Handle, tc.Translation, tc.RotationQuat);
            world.SetVelocity(rb.Handle, rb.Velocity);
        }
    }
}

void PhysicsSystem::SyncPhysicsToEngine(Scene* scene)
{
    auto& registry = scene->GetRegistry();
    auto& world = scene->GetPhysicsWorld();

    auto view = registry.view<RigidBodyComponent, TransformComponent>();
    for (auto entity : view)
    {
        auto& rb = view.get<RigidBodyComponent>(entity);
        auto& tc = view.get<TransformComponent>(entity);

        if (rb.Handle != kInvalidPhysicsBody && !rb.IsKinematic)
        {
            glm::vec3 pos;
            glm::quat rot;
            world.GetTransform(rb.Handle, pos, rot);

            tc.Translation = pos;
            tc.SetRotationQuat(rot);
            rb.Velocity = world.GetVelocity(rb.Handle);
        }
    }
}

} // namespace CHEngine

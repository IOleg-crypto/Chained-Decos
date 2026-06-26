
#include "physics.h"
#include "engine/core/log.h"
#include "engine/assets/types/model_asset.h"
#include "engine/assets/asset_manager.h"
#include "engine/physics/bvh/bvh_cache.h"
#include "engine/scene/components.h"
#include "engine/project/project.h"
#include "engine/scene/scene.h"
#include "iphysics_world.h"
#include "jolt_physics_world.h"
#include "collision_core.h"
#include "raycast_query.h"
#include "dynamics.h"
#include "engine/core/service_locator.h"

// Jolt includes for global initialization
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>

#include <utility>
#include <vector>

namespace Chained
{
namespace
{
BVHCache& Cache()
{
    static BVHCache s_Cache;
    return s_Cache;
}
} // namespace

Physics* Physics::s_Instance = nullptr;

Physics::Physics() = default;
Physics::~Physics() = default;

void Physics::Initialize()
{
    if (s_Instance)
        return;

    // s_Instance points to *this* object, managed by ServiceLocator.
    s_Instance = this;

    // Global Jolt Initialization
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    Cache().Init();
    CH_CORE_INFO("Physics initialized (Jolt backend).");
}

void Physics::Shutdown()
{
    if (!s_Instance)
        return;

    Cache().Shutdown();

    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    // Do NOT delete s_Instance — ServiceLocator owns the lifetime.
    s_Instance = nullptr;
    CH_CORE_INFO("Physics shutdown.");
}


bool Physics::IsInitialized()
{
    return s_Instance != nullptr;
}

IPhysicsWorld* Physics::GetWorld()
{
    if (!m_World)
    {
        m_World = std::make_unique<JoltPhysicsWorld>();
    }
    return m_World.get();
}

void Physics::InitializeBodies(Scene* scene)
{
    auto world = GetWorld();
    if (!world) return;

    auto& registry = scene->GetRegistry();
    auto view = registry.view<TransformComponent, RigidBodyComponent>();
    for (auto entity : view)
    {
        auto [transform, rb] = view.get<TransformComponent, RigidBodyComponent>(entity);
        if (rb.Handle != kInvalidPhysicsBody) continue;

        PhysicsBodyDesc desc;
        desc.Position = transform.Translation;
        desc.Rotation = transform.RotationQuat;
        desc.IsKinematic = rb.IsKinematic;
        desc.Mass = rb.Mass;
        desc.UseGravity = rb.UseGravity;
        desc.InitialVelocity = rb.Velocity;
        desc.Shape = ColliderType::Box; 
        desc.Dimensions = transform.Scale;

        rb.Handle = world->CreateBody(desc);
    }
}

std::shared_ptr<BVH> Physics::GetBVH(const std::shared_ptr<ModelAsset>& asset)
{
    return Cache().GetOrBuild(asset);
}

std::shared_ptr<BVH> Physics::GetBVH(const std::string& modelPath)
{
    auto& am = (*ServiceLocator::Get<AssetManager>());
    auto handle = am.ResolveToHandle(modelPath, AssetType::Model);
    if (handle == AssetHandle(0)) return nullptr;

    auto asset = am.GetAsset<ModelAsset>(handle);
    if (asset) return GetBVH(asset);
    return nullptr;
}

void Physics::InvalidateBVH(const std::string& path)
{
    Cache().Invalidate(path);
}

void Physics::UpdateBVHCache(const std::string& path, std::shared_ptr<BVH> bvh)
{
    Cache().Put(path, bvh);
}

void Physics::Update(Scene* scene, Timestep deltaTime, bool runtime)
{
    if (!runtime) return;

    PhysicsContext& ctx = GetContext(scene);
    ctx.Accumulator += deltaTime;
    
    // Fixed Timestep is standard 60fps for now
    const float FixedTimestep = 1.0f / 60.0f;

    while (ctx.Accumulator >= FixedTimestep)
    {
        // 2. Perform Physics Step (Jolt/Native)
        if (auto world = ServiceLocator::Get<Physics>()->GetWorld())
        {
            world->Step(FixedTimestep);
        }

        // 3. Update Transforms from Physics
        UpdateColliders(scene);

        ctx.Accumulator -= FixedTimestep;
    }
}

RaycastResult Physics::Raycast(Scene* scene, Ray ray)
{
    if (auto world = ServiceLocator::Get<Physics>()->GetWorld())
    {
        return world->Raycast(ray.position, ray.direction, 1000.0f);
    }
    return {};
}

PhysicsContext& Physics::GetContext(Scene* scene)
{
    auto& reg = scene->GetRegistry();
    if (!reg.ctx().contains<PhysicsContext>())
    {
        reg.ctx().emplace<PhysicsContext>();
    }
    return reg.ctx().get<PhysicsContext>();
}

void Physics::ResetAccumulator(Scene* scene)
{
    GetContext(scene).Accumulator = 0.0f;
}

void Physics::ClearContext(Scene* scene)
{
    scene->GetRegistry().ctx().erase<PhysicsContext>();
}

void Physics::SetCollisionCallback(Scene* scene, std::function<void(entt::entity, entt::entity)> callback)
{
    GetContext(scene).CollisionCallback = callback;
}

void Physics::UpdateColliders(Scene* scene)
{
    auto world = ServiceLocator::Get<Physics>()->GetWorld();
    auto& registry = scene->GetRegistry();
    auto view = registry.view<TransformComponent, RigidBodyComponent>();
    for (auto entity : view)
    {
        auto [transform, rb] = view.get<TransformComponent, RigidBodyComponent>(entity);
        if (rb.Handle == kInvalidPhysicsBody) continue;

        glm::vec3 pos;
        glm::quat rot;
        world->GetTransform(rb.Handle, pos, rot);
        transform.Translation = pos;
        transform.RotationQuat = rot;
    }
}

void Physics::ResolveSimulation(Scene* scene, Timestep deltaTime, float gravity)
{
    // Integration logic here if needed
}

} // namespace Chained

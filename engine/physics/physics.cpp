
#include "engine/core/profiler.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/model_asset.h"
#include "engine/physics/bvh/bvh.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "narrow_phase.h"
#include "scene_trace.h"
#include "dynamics.h"
#include <mutex>
#include <unordered_map>

#include "engine/core/application.h"

namespace CHEngine
{
static PhysicsSystem* s_PhysicsInstance = nullptr;

PhysicsSystem::PhysicsSystem()
{
    CH_CORE_ASSERT(!s_PhysicsInstance, "PhysicsSystem already exists!");
    s_PhysicsInstance = this;
}

PhysicsSystem::~PhysicsSystem()
{
    Shutdown();
    s_PhysicsInstance = nullptr;
}

void PhysicsSystem::Init()
{
    CH_CORE_INFO("Global Physics System Initialized.");
}

void PhysicsSystem::Shutdown()
{
    std::lock_guard<std::mutex> lock(m_BVHMutex);
    m_BVHCache.clear();
    CH_CORE_INFO("Global Physics System Shutdown.");
}

PhysicsSystem& PhysicsSystem::Get()
{
    CH_CORE_ASSERT(s_PhysicsInstance, "PhysicsSystem not initialized!");
    return *s_PhysicsInstance;
}

std::shared_ptr<BVH> PhysicsSystem::GetBVH(ModelAsset* asset)
{
    if (!asset || asset->GetState() != AssetState::Ready)
    {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(m_BVHMutex);

    auto it = m_BVHCache.find(asset);
    if (it == m_BVHCache.end())
    {
        m_BVHCache[asset] =
            BVH::BuildAsync(asset->GetModel(), asset->GetGlobalNodeTransforms(), asset->GetMeshToNode()).share();
        return nullptr;
    }

    if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        return it->second.get();
    }

    return nullptr;
}

void PhysicsSystem::InvalidateBVH(ModelAsset* asset)
{
    if (!asset) return;
    std::lock_guard<std::mutex> lock(m_BVHMutex);
    m_BVHCache.erase(asset);
}

void PhysicsSystem::UpdateBVHCache(ModelAsset* asset, std::shared_ptr<BVH> bvh)
{
    if (!asset || !bvh) return;
    std::lock_guard<std::mutex> lock(m_BVHMutex);

    std::promise<std::shared_ptr<BVH>> promise;
    promise.set_value(bvh);
    m_BVHCache[asset] = promise.get_future().share();
}

PhysicsContext& Physics::GetContext(Scene* scene)
{
    auto& registry = scene->GetRegistry();
    auto* ctx = registry.ctx().find<PhysicsContext>();
    if (!ctx)
    {
        return registry.ctx().emplace<PhysicsContext>();
    }
    return *ctx;
}

void Physics::SetCollisionCallback(Scene* scene, std::function<void(entt::entity, entt::entity)> callback)
{
    GetContext(scene).CollisionCallback = callback;
}

void Physics::Update(Scene* scene, Timestep deltaTime, bool runtime)
{
    CH_PROFILE_FUNCTION();
    if (!scene) return;

    auto& registry = scene->GetRegistry();
    auto collView = registry.view<ColliderComponent>();
    for (auto entity : collView)
        collView.get<ColliderComponent>(entity).IsColliding = false;

    if (!runtime)
        return;

    UpdateColliders(scene);

    float fixedTimestep = 1.0f / 60.0f;
    if (auto project = Project::GetActive())
        fixedTimestep = project->GetConfig().Physics.FixedTimestep;

    auto& context = GetContext(scene);
    context.Accumulator += deltaTime;
    while (context.Accumulator >= fixedTimestep)
    {
        ResolveSimulation(scene, fixedTimestep);
        context.Accumulator -= fixedTimestep;
    }
}

RaycastResult Physics::Raycast(Scene* scene, Ray ray)
{
    if (!scene) return RaycastResult();
    return SceneTrace::Raycast(scene->GetRegistry(), ray);
}

void Physics::UpdateColliders(Scene* scene)
{
    auto& registry = scene->GetRegistry();
    auto project = Project::GetActive();
    if (!project)
        return;

    auto genView = registry.view<ColliderComponent, TransformComponent>();

    for (auto entity : genView)
    {
        auto& collider = genView.get<ColliderComponent>(entity);

        // Case A: Box Collider (Auto)
        if (collider.Type == ColliderType::Box && collider.AutoCalculate)
        {
            if (!registry.all_of<ModelComponent>(entity))
                continue;

            auto& model = registry.get<ModelComponent>(entity);
            auto asset = AssetManager::Get().Get<ModelAsset>(model.ModelPath);

            if (asset && asset->GetState() == AssetState::Ready)
            {
                if (collider.Size.x == 0 && collider.Size.y == 0 && collider.Size.z == 0)
                {
                    BoundingBox box = asset->GetBoundingBox();
                    collider.Size   = Vector3Subtract(box.max, box.min);
                    collider.Offset = box.min;
                }
            }
            continue;
        }

        // Case B: Mesh Collider (BVH)
        if (collider.Type == ColliderType::Mesh && !collider.ModelPath.empty())
        {
            auto asset = AssetManager::Get().Get<ModelAsset>(collider.ModelPath);

            if (asset && asset->GetState() == AssetState::Ready && asset->GetModel().meshCount > 0)
            {
                if (collider.AutoCalculate && collider.Size.x == 0)
                {
                    BoundingBox box = asset->GetBoundingBox();
                    collider.Offset = box.min;
                    collider.Size   = Vector3Subtract(box.max, box.min);
                }
            }
            continue;
        }

        // Case C: Sphere Collider (Auto)
        if (collider.Type == ColliderType::Sphere && collider.AutoCalculate)
        {
            if (!registry.all_of<ModelComponent>(entity))
                continue;

            auto& model = registry.get<ModelComponent>(entity);
            auto asset = AssetManager::Get().Get<ModelAsset>(model.ModelPath);

            if (asset && asset->GetState() == AssetState::Ready && collider.Radius == 0)
            {
                BoundingBox box = asset->GetBoundingBox();
                Vector3 size = Vector3Subtract(box.max, box.min);
                collider.Radius = fmaxf(size.x, fmaxf(size.y, size.z)) * 0.5f;
                collider.Offset = Vector3Scale(Vector3Add(box.min, box.max), 0.5f);
            }
            continue;
        }
    }
}

void Physics::ResolveSimulation(Scene* scene, Timestep deltaTime)
{
    auto& registry = scene->GetRegistry();
    auto rbView = registry.view<TransformComponent, RigidBodyComponent>();
    std::vector<entt::entity> rbEntities;
    rbEntities.reserve(rbView.size_hint());

    for (auto entity : rbView)
    {
        rbEntities.push_back(entity);
    }

    if (rbEntities.empty())
        return;

    Dynamics::Update(registry, rbEntities, deltaTime);
    NarrowPhase::ResolveCollisions(registry, rbEntities);
}

} // namespace CHEngine

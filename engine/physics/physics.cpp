#include "engine/core/profiler.h"
#include "physics.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "engine/core/log.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/physics/bvh/bvh_cache.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "collision_core.h"
#include "raycast_query.h"
#include "dynamics.h"
#include "engine/scene/components/component_utils.h"
#include <utility>
#include <vector>

namespace CHEngine
{
namespace
{
BVHCache& Cache()
{
    static BVHCache s_Cache;
    return s_Cache;
}

bool& InitializedFlag()
{
    static bool s_Initialized = false;
    return s_Initialized;
}
} // namespace

void Physics::Init()
{
    if (InitializedFlag())
    {
        return;
    }

    Cache().Init();
    InitializedFlag() = true;
    CH_CORE_INFO("Physics initialized.");
}

void Physics::Shutdown()
{
    if (!InitializedFlag())
    {
        return;
    }

    Cache().Shutdown();
    InitializedFlag() = false;
    CH_CORE_INFO("Physics shutdown.");
}

bool Physics::IsInitialized()
{
    return InitializedFlag();
}

std::shared_ptr<BVH> Physics::GetBVH(const std::string& path)
{
    if (!InitializedFlag())
    {
        return nullptr;
    }

    return Cache().GetOrBuild(path);
}

void Physics::InvalidateBVH(const std::string& path)
{
    if (!InitializedFlag())
    {
        return;
    }

    Cache().Invalidate(path);
}

void Physics::UpdateBVHCache(const std::string& path, std::shared_ptr<BVH> bvh)
{
    if (!InitializedFlag())
    {
        return;
    }

    Cache().Put(path, std::move(bvh));
}

PhysicsContext& Physics::GetContext(Scene* scene)
{
    auto& registry = scene->GetRegistry();
    auto* ctx = registry.ctx().find<PhysicsContext>();
    if (!ctx) return registry.ctx().emplace<PhysicsContext>();
    return *ctx;
}

void Physics::ResetAccumulator(Scene* scene)
{
    if (!scene)
    {
        return;
    }

    GetContext(scene).Accumulator = 0.0f;
}

void Physics::ClearContext(Scene* scene)
{
    if (!scene)
    {
        return;
    }

    scene->GetRegistry().ctx().erase<PhysicsContext>();
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
    
    // Update scene statistics
    ProfilerStats stats = Profiler::GetStats();
    stats.ColliderCount = (uint32_t)collView.size();
    Profiler::UpdateStats(stats);

    for (auto it = collView.begin(); it != collView.end(); ++it)
    {
        auto entity = *it;
        collView.get<ColliderComponent>(entity).IsColliding = false;
    }

    UpdateColliders(scene);

    if (!runtime) return;

    float fixedTimestep = 1.0f / 120.0f;
    if (auto project = Project::GetActive())
    {
        float cfg = project->GetConfig().Physics.FixedTimestep;
        if (cfg > 0.0f) fixedTimestep = cfg;
    }

    auto& context = GetContext(scene);
    context.Accumulator += (float)deltaTime;

    const float maxAccumulator = 0.2f;
    if (context.Accumulator > maxAccumulator) context.Accumulator = maxAccumulator;

    while (context.Accumulator >= fixedTimestep)
    {
        ResolveSimulation(scene, fixedTimestep);
        context.Accumulator -= fixedTimestep;
    }
}

RaycastResult Physics::Raycast(Scene* scene, Ray ray)
{
    if (!scene) return RaycastResult();
    return RaycastQuery::Raycast(scene->GetRegistry(), ray);
}

void Physics::UpdateColliders(Scene* scene)
{
    auto& registry = scene->GetRegistry();
    auto project = Project::GetActive();
    if (!project) return;

    auto genView = registry.view<ColliderComponent, TransformComponent>();

    for (auto it = genView.begin(); it != genView.end(); ++it)
    {
        auto entity = *it;
        auto& collider = genView.get<ColliderComponent>(entity);

        if (collider.Type == ColliderType::Box && collider.AutoCalculate)
        {
            if (!registry.all_of<ModelComponent>(entity)) continue;
            auto& model = registry.get<ModelComponent>(entity);
                auto handle = ServiceLocator::Get<AssetManager>().ResolveToHandle(model.ModelPath, ModelAsset::GetStaticType());
                auto asset = ServiceLocator::Get<AssetManager>().Get<ModelAsset>(handle);

            if (asset && asset->GetState() == AssetState::Ready)
            {
                BoundingBox box = asset->GetBoundingBox();
                collider.Size = box.Max - box.Min;
                collider.Offset = box.Min;
            }
        }
        else if (collider.Type == ColliderType::Sphere && collider.AutoCalculate)
        {
            if (!registry.all_of<ModelComponent>(entity)) continue;
            auto& model = registry.get<ModelComponent>(entity);
                auto handle = ServiceLocator::Get<AssetManager>().ResolveToHandle(model.ModelPath, ModelAsset::GetStaticType());
                auto asset = ServiceLocator::Get<AssetManager>().Get<ModelAsset>(handle);

            if (asset && asset->GetState() == AssetState::Ready)
            {
                BoundingBox box = asset->GetBoundingBox();
                glm::vec3 sz = box.Max - box.Min;
                collider.Radius = glm::max(sz.x, glm::max(sz.y, sz.z)) * 0.5f;
                collider.Offset = (box.Min + box.Max) * 0.5f;
            }
        }
        else if (collider.Type == ColliderType::Mesh && collider.AutoCalculate)
        {
            if (!registry.all_of<ModelComponent>(entity)) continue;
            auto& model = registry.get<ModelComponent>(entity);
            
            if (!model.ModelPath.empty())
            {
                    auto handle = ServiceLocator::Get<AssetManager>().ResolveToHandle(model.ModelPath, ModelAsset::GetStaticType());
                auto asset = ServiceLocator::Get<AssetManager>().Get<ModelAsset>(handle);
                if (asset && asset->GetState() == AssetState::Ready)
                {
                    collider.ModelHandle = asset->GetID();
                    collider.ModelPath = model.ModelPath;
                    
                    // Pre-warm the BVH cache in editor mode so it's ready for Play Mode
                    Physics::GetBVH(model.ModelPath);
                }
            }
        }
    }
}

void Physics::ResolveSimulation(Scene* scene, Timestep deltaTime)
{
    auto& registry = scene->GetRegistry();
    auto rbView = registry.view<TransformComponent, RigidBodyComponent>();
    std::vector<entt::entity> rbEntities;
    rbEntities.reserve(rbView.size_hint());

    for (auto it = rbView.begin(); it != rbView.end(); ++it)
    {
        rbEntities.push_back(*it);
    }

    if (!rbEntities.empty())
    {
        Dynamics::Update(registry, rbEntities, deltaTime);
        
        // CRITICAL: Update WorldTransform for dynamic entities after movement so that
        // CollisionCore::GenerateContacts uses up-to-date broadphase AABBs.
        for (auto entity : rbEntities)
        {
            auto& tc = registry.get<TransformComponent>(entity);
            auto* hc = registry.try_get<HierarchyComponent>(entity);
            
            if (!hc || hc->Parent == entt::null || !registry.valid(hc->Parent) || !registry.all_of<TransformComponent>(hc->Parent))
            {
                 tc.WorldTransform = ComponentUtils::GetTransform(tc);
            }
            else
            {
                 auto& parentTC = registry.get<TransformComponent>(hc->Parent);
                 tc.WorldTransform = parentTC.WorldTransform * ComponentUtils::GetTransform(tc);
            }
            tc.InverseWorldTransform = glm::inverse(tc.WorldTransform);
        }

        CollisionCore::ResolveCollisions(registry, rbEntities);
    }
}
} // namespace CHEngine

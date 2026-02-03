#include "physics.h"
#include "dynamics.h"
#include "engine/core/profiler.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/model_asset.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "narrow_phase.h"
#include "scene_trace.h"


#include "engine/physics/bvh/bvh.h"
#include <unordered_map>
#include <mutex>

namespace CHEngine
{
static std::unordered_map<ModelAsset*, std::shared_future<std::shared_ptr<BVH>>> s_BVHFutureCache;
static std::mutex s_BVHMutex;

void Physics::Init()
{
}
void Physics::Shutdown()
{
}

void Physics::Update(Scene *scene, float deltaTime, bool runtime)
{
    CH_PROFILE_FUNCTION();
    auto &registry = scene->GetRegistry();

    // 1. Diagnostics & Stats
    ProfilerStats stats;
    stats.EntityCount = (uint32_t)registry.storage<entt::entity>().size();
    stats.ColliderCount = (uint32_t)registry.view<ColliderComponent>().size();
    Profiler::UpdateStats(stats);

    // 2. State Reset
    auto collView = registry.view<ColliderComponent>();
    for (auto entity : collView)
        collView.get<ColliderComponent>(entity).IsColliding = false;

    // 3. Collider Generation (Auto-calculate boxes/link BVH)
    auto genView = registry.view<ColliderComponent, TransformComponent>();
    for (auto entity : genView)
    {
        auto &collider = genView.get<ColliderComponent>(entity);
        if (collider.Type == ColliderType::Box && collider.AutoCalculate)
        {
            if (registry.all_of<ModelComponent>(entity))
            {
                auto &model = registry.get<ModelComponent>(entity);
                auto asset = AssetManager::Get<ModelAsset>(model.ModelPath);
                if (asset && asset->GetState() == AssetState::Ready)
                {
                    BoundingBox box = asset->GetBoundingBox();
                    collider.Size = Vector3Subtract(box.max, box.min);
                    collider.Offset = box.min;
                    collider.AutoCalculate = false;
                }
            }
        }
        else if (collider.Type == ColliderType::Mesh && !collider.BVHRoot &&
                 !collider.ModelPath.empty())
        {
            auto asset = AssetManager::Get<ModelAsset>(collider.ModelPath);
            if (asset && asset->GetState() == AssetState::Ready && asset->GetModel().meshCount > 0)
            {
                collider.BVHRoot = Physics::GetBVH(asset.get());
                if (collider.BVHRoot)
                {
                    BoundingBox box = asset->GetBoundingBox();
                    collider.Offset = box.min;
                    collider.Size = Vector3Subtract(box.max, box.min);
                }
            }
        }
    }

    if (!runtime)
        return;

    // 4. Collect Simulation Entities
    auto rbView = registry.view<TransformComponent, RigidBodyComponent>();
    std::vector<entt::entity> rbEntities;
    rbEntities.reserve(rbView.size_hint());
    for (auto entity : rbView)
        rbEntities.push_back(entity);

    if (rbEntities.empty())
        return;

    // 5. Simulation & Resolution
    Dynamics::Update(scene, rbEntities, deltaTime);
    NarrowPhase::ResolveCollisions(scene, rbEntities);
}

RaycastResult Physics::Raycast(Scene *scene, Ray ray)
{
    return SceneTrace::Raycast(scene, ray);
}

std::shared_ptr<BVH> Physics::GetBVH(ModelAsset *asset)
{
    if (!asset || asset->GetState() != AssetState::Ready) return nullptr;

    std::lock_guard<std::mutex> lock(s_BVHMutex);
    
    auto it = s_BVHFutureCache.find(asset);
    if (it == s_BVHFutureCache.end())
    {
        // Start building in background
        s_BVHFutureCache[asset] = BVH::BuildAsync(asset->GetModel()).share();
        return nullptr;
    }

    // Check if ready
    if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        return it->second.get();
    }

    return nullptr;
}
} // namespace CHEngine

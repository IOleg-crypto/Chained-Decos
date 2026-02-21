#include "physics.h"
#include "dynamics.h"
#include "engine/core/profiler.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/model_asset.h"
#include "engine/physics/bvh/bvh.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "narrow_phase.h"
#include "scene_trace.h"
#include <mutex>
#include <unordered_map>

namespace CHEngine
{
Physics::Physics(Scene* scene)
    : m_Scene(scene)
{
    m_NarrowPhase = std::make_unique<NarrowPhase>(this);
    m_Dynamics = std::make_unique<Dynamics>();
    m_SceneTrace = std::make_unique<SceneTrace>(this);
}

Physics::~Physics()
{
    CH_CORE_INFO("Physics instance for scene destroyed.");
}

void Physics::Init()
{
    CH_CORE_INFO("Global Physics System Initialized.");
}

void Physics::Shutdown()
{
    CH_CORE_INFO("Global Physics System Shutdown.");
}

void Physics::Update(Timestep deltaTime, bool runtime)
{
    CH_PROFILE_FUNCTION();
    CH_CORE_ASSERT(m_Scene, "Physics Scene is null!");

    // 1. Diagnostics & Stats
    UpdateColliders();

    if (!runtime)
    {
        return;
    }

    // 3. Simulation & Resolution (Fixed Timestep)
    float fixedTimestep = 1.0f / 60.0f;
    auto project = Project::GetActive();
    if (project)
    {
        fixedTimestep = project->GetConfig().Physics.FixedTimestep;
    }

    m_Accumulator += deltaTime;
    while (m_Accumulator >= fixedTimestep)
    {
        ResolveSimulation(fixedTimestep);
        m_Accumulator -= fixedTimestep;
    }
}

RaycastResult Physics::Raycast(Ray ray)
{
    CH_CORE_ASSERT(m_Scene, "Physics Scene is null!");
    return m_SceneTrace->Raycast(m_Scene, ray);
}

std::shared_ptr<BVH> Physics::GetBVH(ModelAsset* asset)
{
    if (!asset || asset->GetState() != AssetState::Ready)
    {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(m_BVHMutex);

    auto it = m_BVHCache.find(asset);
    if (it == m_BVHCache.end())
    {
        // Start building in background using full model data (node transforms)
        m_BVHCache[asset] =
            BVH::BuildAsync(asset->GetModel(), asset->GetGlobalNodeTransforms(), asset->GetMeshToNode()).share();
        return nullptr;
    }

    // Check if ready
    if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        return it->second.get();
    }

    return nullptr;
}

void Physics::InvalidateBVH(ModelAsset* asset)
{
    if (!asset)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(m_BVHMutex);
    m_BVHCache.erase(asset);
}

void Physics::UpdateBVHCache(ModelAsset* asset, std::shared_ptr<BVH> bvh)
{
    if (!asset || !bvh)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(m_BVHMutex);

    std::promise<std::shared_ptr<BVH>> promise;
    promise.set_value(bvh);
    m_BVHCache[asset] = promise.get_future().share();
}

void Physics::UpdateColliders()
{
    auto& registry = m_Scene->GetRegistry();

    // 1. Reset Collision State
    auto collView = registry.view<ColliderComponent>();
    for (auto entity : collView)
    {
        collView.get<ColliderComponent>(entity).IsColliding = false;
    }

    // 2. Auto-Calculate Colliders from Assets
    auto project = Project::GetActive();
    if (!project || !project->GetAssetManager())
    {
        return;
    }

    auto genView = registry.view<ColliderComponent, TransformComponent>();
    for (auto entity : genView)
    {
        auto& collider = genView.get<ColliderComponent>(entity);

        // Case A: Box Collider (Auto)
        if (collider.Type == ColliderType::Box && collider.AutoCalculate)
        {
            if (!registry.all_of<ModelComponent>(entity))
            {
                continue;
            }

            auto& model = registry.get<ModelComponent>(entity);
            auto asset = project->GetAssetManager()->Get<ModelAsset>(model.ModelPath);

            if (asset && asset->GetState() == AssetState::Ready)
            {
                BoundingBox box = asset->GetBoundingBox();
                collider.Size = Vector3Subtract(box.max, box.min);
                collider.Offset = box.min;
            }
            continue;
        }

        // Case B: Mesh Collider (BVH)
        if (collider.Type == ColliderType::Mesh && !collider.ModelPath.empty())
        {
            auto asset = project->GetAssetManager()->Get<ModelAsset>(collider.ModelPath);
            if (asset && asset->GetState() == AssetState::Ready && asset->GetModel().meshCount > 0)
            {
                // Always try to fetch BVH - if asset changed, GetBVH will return the new one (or null if building)
                auto bvh = GetBVH(asset.get());
                if (bvh)
                {
                    collider.BVHRoot = bvh;
                }

                if (collider.AutoCalculate && collider.BVHRoot)
                {
                    BoundingBox box = asset->GetBoundingBox();
                    collider.Offset = box.min;
                    collider.Size = Vector3Subtract(box.max, box.min);
                }
            }
            continue;
        }

        // Case C: Sphere Collider (Auto)
        if (collider.Type == ColliderType::Sphere && collider.AutoCalculate)
        {
            if (!registry.all_of<ModelComponent>(entity))
            {
                continue;
            }

            auto& model = registry.get<ModelComponent>(entity);
            auto asset = project->GetAssetManager()->Get<ModelAsset>(model.ModelPath);

            if (asset && asset->GetState() == AssetState::Ready)
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

void Physics::ResolveSimulation(Timestep deltaTime)
{
    auto& registry = m_Scene->GetRegistry();
    auto rbView = registry.view<TransformComponent, RigidBodyComponent>();
    std::vector<entt::entity> rbEntities;
    rbEntities.reserve(rbView.size_hint());

    for (auto entity : rbView)
    {
        rbEntities.push_back(entity);
    }

    if (rbEntities.empty())
    {
        return;
    }

    m_Dynamics->Update(m_Scene, rbEntities, deltaTime);
    m_NarrowPhase->ResolveCollisions(m_Scene, rbEntities);
}

} // namespace CHEngine

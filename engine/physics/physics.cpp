#include "physics.h"
#include "dynamics.h"
#include "engine/core/profiler.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/model_asset.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "engine/scene/project.h"
#include "narrow_phase.h"
#include "scene_trace.h"
#include "engine/physics/bvh/bvh.h"
#include <unordered_map>
#include <mutex>

namespace CHEngine
{
    Physics::Physics(Scene* scene)
        : m_Scene(scene)
    {
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

    void Physics::Update(float deltaTime, bool runtime)
    {
        CH_PROFILE_FUNCTION();
        CH_CORE_ASSERT(m_Scene, "Physics Scene is null!");

        // 1. Diagnostics & Stats
        UpdateColliders();

        if (!runtime)
            return;

        // 2. Collect Simulation Entities
        auto& registry = m_Scene->GetRegistry();
        auto rbView = registry.view<TransformComponent, RigidBodyComponent>();
        std::vector<entt::entity> rbEntities;
        rbEntities.reserve(rbView.size_hint());
        
        for (auto entity : rbView)
            rbEntities.push_back(entity);

        if (rbEntities.empty())
            return;

        // 3. Simulation & Resolution
        ResolveSimulation(deltaTime);
    }

    RaycastResult Physics::Raycast(Ray ray)
    {
        CH_CORE_ASSERT(m_Scene, "Physics Scene is null!");
        return SceneTrace::Raycast(m_Scene, ray, this);
    }

    std::shared_ptr<BVH> Physics::GetBVH(ModelAsset* asset)
    {
        if (!asset || asset->GetState() != AssetState::Ready) return nullptr;

        std::lock_guard<std::mutex> lock(m_BVHMutex);
        
        auto it = m_BVHCache.find(asset);
        if (it == m_BVHCache.end())
        {
            // Start building in background
            m_BVHCache[asset] = BVH::BuildAsync(asset->GetModel()).share();
            return nullptr;
        }

        // Check if ready
        if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            return it->second.get();
        }

        return nullptr;
    }

    void Physics::UpdateColliders()
    {
        auto& registry = m_Scene->GetRegistry();
        
        // 1. Reset Collision State
        auto collView = registry.view<ColliderComponent>();
        for (auto entity : collView)
            collView.get<ColliderComponent>(entity).IsColliding = false;

        // 2. Auto-Calculate Colliders from Assets
        auto project = Project::GetActive();
        if (!project || !project->GetAssetManager()) return;

        auto genView = registry.view<ColliderComponent, TransformComponent>();
        for (auto entity : genView)
        {
            auto &collider = genView.get<ColliderComponent>(entity);
            
            // Case A: Box Collider (Auto)
            if (collider.Type == ColliderType::Box && collider.AutoCalculate)
            {
                if (!registry.all_of<ModelComponent>(entity)) continue;

                auto &model = registry.get<ModelComponent>(entity);
                auto asset = project->GetAssetManager()->Get<ModelAsset>(model.ModelPath);
                
                if (asset && asset->GetState() == AssetState::Ready)
                {
                    BoundingBox box = asset->GetBoundingBox();
                    collider.Size = Vector3Subtract(box.max, box.min);
                    collider.Offset = box.min;
                    collider.AutoCalculate = false;
                }
                continue;
            }

            // Case B: Mesh Collider (BVH)
            if (collider.Type == ColliderType::Mesh && !collider.ModelPath.empty())
            {
                if (collider.BVHRoot) continue; // Already generated

                auto asset = project->GetAssetManager()->Get<ModelAsset>(collider.ModelPath);
                if (asset && asset->GetState() == AssetState::Ready && asset->GetModel().meshCount > 0)
                {
                    collider.BVHRoot = GetBVH(asset.get());
                    if (collider.BVHRoot)
                    {
                        BoundingBox box = asset->GetBoundingBox();
                        collider.Offset = box.min;
                        collider.Size = Vector3Subtract(box.max, box.min);
                    }
                }
            }
        }
    }

    void Physics::ResolveSimulation(float deltaTime)
    {
        auto& registry = m_Scene->GetRegistry();
        auto rbView = registry.view<TransformComponent, RigidBodyComponent>();
        std::vector<entt::entity> rbEntities;
        
        for (auto entity : rbView)
            rbEntities.push_back(entity);

        Dynamics::Update(m_Scene, rbEntities, deltaTime);
        NarrowPhase::ResolveCollisions(m_Scene, rbEntities);
    }

} // namespace CHEngine

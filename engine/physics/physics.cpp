
#include "engine/core/profiler.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/physics/bvh/bvh.h"
#include "physics.h"
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
    InternalShutdown();
    s_PhysicsInstance = nullptr;
}

void PhysicsSystem::Init()
{
    if (!s_PhysicsInstance)
        s_PhysicsInstance = new PhysicsSystem();
    s_PhysicsInstance->InternalInit();
}

void PhysicsSystem::InternalInit()
{
    CH_CORE_INFO("Global Physics System Initialized.");
}

void PhysicsSystem::Shutdown()
{
    if (s_PhysicsInstance)
    {
        s_PhysicsInstance->InternalShutdown();
        delete s_PhysicsInstance;
        s_PhysicsInstance = nullptr;
    }
}

void PhysicsSystem::InternalShutdown()
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

std::shared_ptr<BVH> PhysicsSystem::GetBVH(const std::string& path)
{
    if (path.empty()) return nullptr;
    
    auto asset = AssetManager::Get().Get<ModelAsset>(path);
    if (!asset || asset->GetState() != AssetState::Ready)
    {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(m_BVHMutex);

    auto it = m_BVHCache.find(path);
    if (it == m_BVHCache.end())
    {
        CH_CORE_INFO("PhysicsSystem: Starting synchronous BVH build for '{}'", asset->GetPath());
        
        const Model& model = asset->GetModel();
        const auto& instances = asset->GetInstances();
        
        std::vector<CollisionTriangle> allTris;
        for (const auto& inst : instances)
        {
            if (inst.meshIndex < 0 || inst.meshIndex >= model.meshCount)
                continue;

            const Mesh& mesh = model.meshes[inst.meshIndex];
            if (mesh.vertexCount == 0 || mesh.vertices == nullptr)
                continue;

            // Local * modelRoot
            Matrix meshTransform = MatrixMultiply(inst.localTransform, model.transform);

            if (mesh.indices != nullptr)
            {
                for (int k = 0; k < mesh.triangleCount * 3; k += 3)
                {
                    uint32_t idx0 = mesh.indices[k];
                    uint32_t idx1 = mesh.indices[k + 1];
                    uint32_t idx2 = mesh.indices[k + 2];

                    allTris.emplace_back(
                        Vector3Transform({mesh.vertices[idx0 * 3], mesh.vertices[idx0 * 3 + 1], mesh.vertices[idx0 * 3 + 2]}, meshTransform),
                        Vector3Transform({mesh.vertices[idx1 * 3], mesh.vertices[idx1 * 3 + 1], mesh.vertices[idx1 * 3 + 2]}, meshTransform),
                        Vector3Transform({mesh.vertices[idx2 * 3], mesh.vertices[idx2 * 3 + 1], mesh.vertices[idx2 * 3 + 2]}, meshTransform),
                        inst.meshIndex
                    );
                }
            }
            else
            {
                for (int k = 0; k < mesh.vertexCount; k += 3)
                {
                    allTris.emplace_back(
                        Vector3Transform({mesh.vertices[k * 3], mesh.vertices[k * 3 + 1], mesh.vertices[k * 3 + 2]}, meshTransform),
                        Vector3Transform({mesh.vertices[(k + 1) * 3], mesh.vertices[(k + 1) * 3 + 1], mesh.vertices[(k + 1) * 3 + 2]}, meshTransform),
                        Vector3Transform({mesh.vertices[(k + 2) * 3], mesh.vertices[(k + 2) * 3 + 1], mesh.vertices[(k + 2) * 3 + 2]}, meshTransform),
                        inst.meshIndex
                    );
                }
            }
        }

        auto bvh = BVH::Build(std::move(allTris));
        
        if (!bvh->GetTriangles().empty())
        {
            const auto& tri = bvh->GetTriangles()[0];
            CH_CORE_TRACE("BVH Built for '{}': Tri0.v0 = ({}, {}, {})", path, tri.v0.x, tri.v0.y, tri.v0.z);
        }
        
        std::promise<std::shared_ptr<BVH>> promise;
        promise.set_value(bvh);
        m_BVHCache[path] = promise.get_future().share();
        
        return bvh;
    }

    // Return current cached value (gets blocking wait if still building async)
    if (it->second.valid())
    {
        return it->second.get();
    }

    return nullptr;
}

void PhysicsSystem::InvalidateBVH(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_BVHMutex);
    m_BVHCache.erase(path);
}

void PhysicsSystem::UpdateBVHCache(const std::string& path, std::shared_ptr<BVH> bvh)
{
    if (path.empty() || !bvh) return;
    std::lock_guard<std::mutex> lock(m_BVHMutex);

    std::promise<std::shared_ptr<BVH>> promise;
    promise.set_value(bvh);
    m_BVHCache[path] = promise.get_future().share();
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

    // Update collider sizes/offsets even in editor (for AutoCalculate)
    UpdateColliders(scene);

    if (!runtime)
        return;

    // Fixed timestep: collisions are fully FPS-independent.
    // We pick a small step so penetration per tick is small and stable.
    float fixedTimestep = 1.0f / 120.0f;  // default: 120 Hz physics
    if (auto project = Project::GetActive())
    {
        float cfg = project->GetConfig().Physics.FixedTimestep;
        if (cfg > 0.0f)
            fixedTimestep = cfg;
    }

    auto& context = GetContext(scene);
    context.Accumulator += (float)deltaTime;

    // Clamp accumulator to prevent spiral-of-death on lag spikes
    const float maxAccumulator = 0.2f;
    if (context.Accumulator > maxAccumulator)
        context.Accumulator = maxAccumulator;

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
                BoundingBox box = asset->GetBoundingBox();
                collider.Size   = Vector3Subtract(box.max, box.min);
                collider.Offset = box.min;
            }
            continue;
        }

        // Case B: Mesh Collider (BVH)
        if (collider.Type == ColliderType::Mesh && !collider.ModelPath.empty())
        {
            auto asset = AssetManager::Get().Get<ModelAsset>(collider.ModelPath);

            if (asset && asset->GetState() == AssetState::Ready && asset->GetModel().meshCount > 0)
            {
                if (collider.AutoCalculate)
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

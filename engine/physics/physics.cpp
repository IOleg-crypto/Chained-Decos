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
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
    if (!asset || asset->GetState() != AssetState::Ready) return nullptr;

    std::lock_guard<std::mutex> lock(m_BVHMutex);

    auto it = m_BVHCache.find(path);
    if (it == m_BVHCache.end())
    {
        CH_CORE_INFO("PhysicsSystem: Starting synchronous BVH build for '{}'", asset->GetPath());
        
        const Model& model = asset->GetModel();
        const auto& instances = asset->GetInstances();
        const auto& rawMeshes = asset->GetRawMeshes();
        
        std::vector<CollisionTriangle> allTris;
        for (const auto& inst : instances)
        {
            if (inst.meshIndex < 0 || inst.meshIndex >= (int)rawMeshes.size()) continue;

            const RawMesh& raw = rawMeshes[inst.meshIndex];
            if (raw.indices.size() < 3)
            {
                continue;
            }

            for (size_t i = 0; i + 2 < raw.indices.size(); i += 3)
            {
                uint32_t i0 = raw.indices[i];
                uint32_t i1 = raw.indices[i + 1];
                uint32_t i2 = raw.indices[i + 2];

                size_t v0Idx = (size_t)i0 * 3;
                size_t v1Idx = (size_t)i1 * 3;
                size_t v2Idx = (size_t)i2 * 3;
                if (v0Idx + 2 >= raw.vertices.size() || v1Idx + 2 >= raw.vertices.size() || v2Idx + 2 >= raw.vertices.size())
                {
                    continue;
                }

                glm::vec3 v0 = {raw.vertices[v0Idx], raw.vertices[v0Idx + 1], raw.vertices[v0Idx + 2]};
                glm::vec3 v1 = {raw.vertices[v1Idx], raw.vertices[v1Idx + 1], raw.vertices[v1Idx + 2]};
                glm::vec3 v2 = {raw.vertices[v2Idx], raw.vertices[v2Idx + 1], raw.vertices[v2Idx + 2]};

                // Transform to instance world space (using localTransform which is MeshGlobal in Assimp)
                v0 = glm::vec3(inst.localTransform * glm::vec4(v0, 1.0f));
                v1 = glm::vec3(inst.localTransform * glm::vec4(v1, 1.0f));
                v2 = glm::vec3(inst.localTransform * glm::vec4(v2, 1.0f));

                allTris.emplace_back(v0, v1, v2, inst.meshIndex);
            }
        }
        auto bvh = BVH::Build(std::move(allTris));
        
        std::promise<std::shared_ptr<BVH>> promise;
        promise.set_value(bvh);
        m_BVHCache[path] = promise.get_future().share();
        
        return bvh;
    }
    return it->second.get();
}

void PhysicsSystem::InvalidateBVH(const std::string& path)
{
    if (path.empty()) return;
    std::lock_guard<std::mutex> lock(m_BVHMutex);
    m_BVHCache.erase(path);
}

PhysicsContext& Physics::GetContext(Scene* scene)
{
    auto& registry = scene->GetRegistry();
    auto* ctx = registry.ctx().find<PhysicsContext>();
    if (!ctx) return registry.ctx().emplace<PhysicsContext>();
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
    return SceneTrace::Raycast(scene->GetRegistry(), ray);
}

void Physics::UpdateColliders(Scene* scene)
{
    auto& registry = scene->GetRegistry();
    auto project = Project::GetActive();
    if (!project) return;

    auto genView = registry.view<ColliderComponent, TransformComponent>();

    for (auto entity : genView)
    {
        auto& collider = genView.get<ColliderComponent>(entity);

        if (collider.Type == ColliderType::Box && collider.AutoCalculate)
        {
            if (!registry.all_of<ModelComponent>(entity)) continue;
            auto& model = registry.get<ModelComponent>(entity);
            auto asset = AssetManager::Get().Get<ModelAsset>(model.ModelPath);

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
            auto asset = AssetManager::Get().Get<ModelAsset>(model.ModelPath);

            if (asset && asset->GetState() == AssetState::Ready)
            {
                BoundingBox box = asset->GetBoundingBox();
                glm::vec3 sz = box.Max - box.Min;
                collider.Radius = glm::max(sz.x, glm::max(sz.y, sz.z)) * 0.5f;
                collider.Offset = (box.Min + box.Max) * 0.5f;
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

    for (auto entity : rbView) rbEntities.push_back(entity);

    if (!rbEntities.empty())
    {
        Dynamics::Update(registry, rbEntities, deltaTime);
        NarrowPhase::ResolveCollisions(registry, rbEntities);
    }
}
} // namespace CHEngine

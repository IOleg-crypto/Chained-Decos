#include "physics_body_system.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/model_asset.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"
#include "engine/physics/iphysics_world.h"
#include "engine/physics/physics.h"
#include "engine/scene/components/physics_component.h"
#include "engine/scene/components/model_component.h"
#include "engine/scene/components/transform_component.h"
#include <algorithm>
#include <atomic>
#include <future>

namespace Chained::PhysicsBodySystem
{

void RegisterObservers(entt::registry& reg)
{
    reg.on_construct<RigidBodyComponent>().connect<&OnRigidBodyConstruct>();
}

bool BuildBodyDesc(entt::registry& reg, entt::entity e, PhysicsBodyDesc& outDesc)
{
    if (!reg.all_of<TransformComponent, RigidBodyComponent>(e))
    {
        return false;
    }

    auto& transform = reg.get<TransformComponent>(e);
    auto& rb = reg.get<RigidBodyComponent>(e);

    auto* collider = reg.try_get<ColliderComponent>(e);
    if (!collider || !collider->Enabled)
    {
        return false;
    }

    if (collider->AutoCalculate)
    {
        if (auto* physics = ServiceLocator::TryGet<Physics>())
        {
            physics->ApplyAutoCalculate(e, reg, *collider, transform.Scale);
        }
    }

    PhysicsBodyDesc desc;
    desc.Position = transform.Translation;
    desc.Rotation = transform.RotationQuat;
    desc.IsKinematic = (rb.Type == RigidBodyComponent::BodyType::Kinematic);
    desc.IsStatic = (rb.Type == RigidBodyComponent::BodyType::Static);
    desc.Mass = rb.Mass;
    desc.LinearDamping = rb.LinearDamping;
    desc.AngularDamping = rb.AngularDamping;
    desc.UseGravity = rb.UseGravity;
    desc.IsFixedRotation = rb.IsFixedRotation;
    desc.InitialVelocity = rb.Velocity;
    desc.UserData = (uint64_t)e;

    desc.Shape = collider->Type;
    desc.Friction = collider->Friction;
    desc.Restitution = collider->Restitution;
    desc.Offset = collider->Offset;
    desc.UseFastBuildQuality = collider->UseFastBuildQuality;

    switch (collider->Type)
    {
    case ColliderType::Box:
        desc.Dimensions = (collider->Size * transform.Scale) * 0.5f;
        break;

    case ColliderType::Sphere:
        desc.Dimensions.x = collider->Radius * std::max({transform.Scale.x, transform.Scale.y, transform.Scale.z});
        break;

    case ColliderType::Capsule:
        desc.Dimensions.x = collider->Radius * std::max(transform.Scale.x, transform.Scale.z);
        desc.Dimensions.y = (collider->Height * transform.Scale.y) * 0.5f;
        break;

    case ColliderType::Mesh: {
        std::string modelPath = collider->ModelPath;
        if (modelPath.empty())
        {
            if (auto* modelComp = reg.try_get<ModelComponent>(e))
            {
                modelPath = modelComp->ModelPath;
            }
        }

        if (modelPath.empty())
        {
            return false;
        }

        if (auto* worldPtr = reg.ctx().find<IPhysicsWorld*>())
        {
            if ((*worldPtr) && (*worldPtr)->HasCachedMeshShape(modelPath))
            {
                desc.CacheKey = modelPath;
                desc.MeshScale = transform.Scale;
                break;
            }
        }

        auto* assets = ServiceLocator::TryGet<AssetManager>();
        if (!assets) return false;

        auto modelAsset = assets->Get<ModelAsset>(modelPath);
        if (!modelAsset || !modelAsset->IsReady())
        {
            return false;
        }

        const auto& rawMeshes = modelAsset->GetRawMeshes();
        const auto& instances = modelAsset->GetInstances();

        desc.MeshScale = transform.Scale;
        desc.CacheKey = modelPath;

        for (const auto& inst : instances)
        {
            if (inst.meshIndex < 0 || inst.meshIndex >= (int)rawMeshes.size())
            {
                continue;
            }

            const RawMesh& raw = rawMeshes[inst.meshIndex];
            if (raw.indices.size() < 3)
            {
                continue;
            }

            const glm::mat4& meshToLocal = inst.localTransform;

            for (size_t i = 0; i + 2 < raw.indices.size(); i += 3)
            {
                uint32_t i0 = raw.indices[i];
                uint32_t i1 = raw.indices[i + 1];
                uint32_t i2 = raw.indices[i + 2];

                size_t v0Idx = (size_t)i0 * 3;
                size_t v1Idx = (size_t)i1 * 3;
                size_t v2Idx = (size_t)i2 * 3;

                if (v0Idx + 2 >= raw.vertices.size() || v1Idx + 2 >= raw.vertices.size() ||
                    v2Idx + 2 >= raw.vertices.size())
                {
                    continue;
                }

                glm::vec3 v0 = {raw.vertices[v0Idx], raw.vertices[v0Idx + 1], raw.vertices[v0Idx + 2]};
                glm::vec3 v1 = {raw.vertices[v1Idx], raw.vertices[v1Idx + 1], raw.vertices[v1Idx + 2]};
                glm::vec3 v2 = {raw.vertices[v2Idx], raw.vertices[v2Idx + 1], raw.vertices[v2Idx + 2]};

                PhysicsTriangle tri;
                tri.V0 = glm::vec3(meshToLocal * glm::vec4(v0, 1.0f));
                tri.V1 = glm::vec3(meshToLocal * glm::vec4(v1, 1.0f));
                tri.V2 = glm::vec3(meshToLocal * glm::vec4(v2, 1.0f));

                desc.Triangles.push_back(tri);
            }
        }
        break;
    }
    }

    outDesc = std::move(desc);
    return true;
}

void OnRigidBodyConstruct(entt::registry& reg, entt::entity e)
{
    if (!reg.ctx().contains<IPhysicsWorld*>())
    {
        return;
    }

    auto* worldPtr = reg.ctx().find<IPhysicsWorld*>();
    if (!worldPtr || !(*worldPtr))
    {
        return;
    }

    auto& rb = reg.get<RigidBodyComponent>(e);

    if (rb.Handle != kInvalidPhysicsBody)
    {
        return;
    }

    auto* collider = reg.try_get<ColliderComponent>(e);
    if (!collider || !collider->Enabled)
    {
        return;
    }

    PhysicsBodyDesc desc;
    if (!BuildBodyDesc(reg, e, desc))
    {
        return;
    }

    rb.Handle = (*worldPtr)->CreateBody(desc);
}

void BatchInitializeBodies(entt::registry& reg, IPhysicsWorld* world)
{
    struct PendingBody
    {
        entt::entity entity;
        PhysicsBodyDesc desc;
        int sortOrder;
    };

    std::vector<PendingBody> pending;

    auto view = reg.view<RigidBodyComponent, TransformComponent>();
    for (auto entity : view)
    {
        auto& rb = view.get<RigidBodyComponent>(entity);
        if (rb.Handle != kInvalidPhysicsBody)
        {
            continue;
        }

        auto* collider = reg.try_get<ColliderComponent>(entity);
        if (!collider || !collider->Enabled)
        {
            continue;
        }

        if (collider->AutoCalculate)
        {
            auto& transform = view.get<TransformComponent>(entity);
            if (auto* physics = ServiceLocator::TryGet<Physics>())
            {
                physics->ApplyAutoCalculate(entity, reg, *collider, transform.Scale);
            }
        }

        pending.push_back({entity, {}, 0});
    }

    if (pending.empty())
    {
        return;
    }

    size_t parallelThreshold = 4;
    if (pending.size() >= parallelThreshold)
    {
        std::atomic<bool> buildFailed{false};
        std::vector<std::future<void>> futures;
        futures.reserve(pending.size());

        for (size_t i = 0; i < pending.size(); ++i)
        {
            futures.push_back(std::async(std::launch::async, [&reg, &pending, i, &buildFailed]() {
                if (!BuildBodyDesc(reg, pending[i].entity, pending[i].desc))
                {
                    buildFailed.store(true, std::memory_order_relaxed);
                }
            }));
        }
        for (auto& f : futures)
            f.get();

        if (buildFailed.load())
        {
            pending.erase(
                std::remove_if(pending.begin(), pending.end(),
                               [](const PendingBody& pb) { return pb.desc.Triangles.empty() && pb.desc.Shape == ColliderType::Mesh; }),
                pending.end());
        }
    }
    else
    {
        auto it = pending.begin();
        while (it != pending.end())
        {
            if (!BuildBodyDesc(reg, it->entity, it->desc))
            {
                it = pending.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    for (auto& pb : pending)
    {
        pb.sortOrder = pb.desc.IsStatic ? 0 : (pb.desc.IsKinematic ? 1 : 2);
    }

    if (pending.empty())
    {
        return;
    }

    std::sort(pending.begin(), pending.end(),
              [](const PendingBody& a, const PendingBody& b) { return a.sortOrder < b.sortOrder; });

    std::vector<PhysicsBodyDesc> descs;
    descs.reserve(pending.size());
    for (auto& pb : pending)
    {
        descs.push_back(std::move(pb.desc));
    }

    auto handles = world->CreateBodies(descs);

    for (size_t i = 0; i < pending.size(); ++i)
    {
        auto& rb = reg.get<RigidBodyComponent>(pending[i].entity);
        rb.Handle = handles[i];
    }

    CH_CORE_INFO("Physics: Batch-created {} bodies (static-first).", pending.size());
}

void Update(entt::registry& reg)
{
    CH_PROFILE_FUNCTION();

    reg.view<RigidBodyComponent, TransformComponent>().each([&](auto entity, auto& rb, auto& transform) {
        if (rb.Handle == kInvalidPhysicsBody && reg.ctx().contains<IPhysicsWorld*>())
        {
            OnRigidBodyConstruct(reg, entity);
        }
    });
}

} // namespace Chained::PhysicsBodySystem

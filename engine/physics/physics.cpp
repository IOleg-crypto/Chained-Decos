#include "physics.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/model_asset.h"
#include "engine/core/log.h"
#include "engine/core/service_locator.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "iphysics_world.h"
#include "jolt_physics_world.h"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>

#include "engine/project/project.h"

namespace Chained
{

Physics::Physics() = default;
Physics::~Physics() = default;

void Physics::Initialize()
{
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
    CH_CORE_INFO("Physics initialized (Jolt backend).");
}

void Physics::Shutdown()
{
    m_World.reset();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
    CH_CORE_INFO("Physics shutdown.");
}

IPhysicsWorld* Physics::GetWorld()
{
    if (!m_World)
    {
        m_World = std::make_unique<JoltPhysicsWorld>();
    }
    return m_World.get();
}

void Physics::ResetWorld()
{
    m_World.reset();
    m_World = std::make_unique<JoltPhysicsWorld>();

    if (auto project = Project::GetActive())
    {
        float gravity = project->GetConfig().Physics.Gravity;
        m_World->SetGravity(gravity);
    }

    CH_CORE_INFO("Physics: World reset — fresh Jolt world created.");
}

void Physics::InitializeBodies(Scene* scene)
{
    auto world = GetWorld();
    if (!world)
    {
        return;
    }

    auto& registry = scene->GetRegistry();
    auto view = registry.view<TransformComponent, RigidBodyComponent>();

    for (auto entity : view)
    {
        auto& transform = view.get<TransformComponent>(entity);
        auto& rb = view.get<RigidBodyComponent>(entity);

        if (rb.Handle != kInvalidPhysicsBody)
        {
            continue;
        }

        PhysicsBodyDesc desc;
        desc.Position = transform.Translation;
        desc.Rotation = transform.RotationQuat;
        desc.Mass = rb.Mass;
        desc.LinearDamping = rb.LinearDamping;
        desc.AngularDamping = rb.AngularDamping;
        desc.UseGravity = rb.UseGravity;
        desc.IsKinematic = rb.IsKinematic;
        desc.IsStatic = (rb.Type == RigidBodyComponent::BodyType::Static);
        desc.IsFixedRotation = rb.IsFixedRotation;
        desc.InitialVelocity = rb.Velocity;
        desc.UserData = static_cast<uint64_t>(entity); 

        auto* collider = registry.try_get<ColliderComponent>(entity);
        if (collider && collider->Enabled)
        {
            // ── AutoCalculate: derive dimensions from the model AABB via Jolt ──
            if (collider->AutoCalculate)
            {
                ApplyAutoCalculate(entity, registry, *collider, transform.Scale);
            }

            desc.Shape = collider->Type;
            desc.Friction = collider->Friction;
            desc.Restitution = collider->Restitution;
            desc.Offset = collider->Offset;

            switch (collider->Type)
            {
            case ColliderType::Box:
                desc.Dimensions = collider->Size * transform.Scale * 0.5f;
                break;
            case ColliderType::Sphere:
                desc.Dimensions.x = collider->Radius * glm::compMax(transform.Scale);
                break;
            case ColliderType::Capsule:
                desc.Dimensions.x = collider->Radius;
                desc.Dimensions.y = collider->Height * 0.5f;
                break;
            case ColliderType::Mesh:
                desc.IsStatic = true;
                desc.Offset = collider->Offset * transform.Scale;
                BuildMeshTriangles(collider->ModelPath, transform.Scale, desc.Triangles);
                break;
            }
        }
        else
        {
            desc.Shape = ColliderType::Box;
            desc.Dimensions = transform.Scale * 0.5f;
        }

        rb.Handle = world->CreateBody(desc);
        CH_CORE_INFO("Physics: Created Jolt body handle={} for entity={}. Shape={} Pos=({:.2f},{:.2f},{:.2f})",
                     rb.Handle, (uint32_t)entity, (int)desc.Shape, desc.Position.x, desc.Position.y, desc.Position.z);
    }
    CH_CORE_INFO("Physics: InitializeBodies complete.");
}

void Physics::BuildMeshTriangles(const std::string& modelPath, const glm::vec3& scale,
                                 std::vector<PhysicsTriangle>& outTriangles)
{
    if (modelPath.empty())
    {
        return;
    }

    auto* am = ServiceLocator::Get<AssetManager>();
    auto handle = am->ResolveToHandle(modelPath);
    if (handle == AssetHandle(0))
    {
        return;
    }

    auto asset = am->Get<ModelAsset>(handle);
    if (!asset || asset->GetState() != AssetState::Ready)
    {
        CH_CORE_WARN("Physics: Model '{}' not ready for mesh collider.", modelPath);
        return;
    }

    const auto& instances = asset->GetInstances();
    const auto& rawMeshes = asset->GetRawMeshes();

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

        for (size_t i = 0; i + 2 < raw.indices.size(); i += 3)
        {
            uint32_t i0 = raw.indices[i];
            uint32_t i1 = raw.indices[i + 1];
            uint32_t i2 = raw.indices[i + 2];

            size_t v0 = (size_t)i0 * 3;
            size_t v1 = (size_t)i1 * 3;
            size_t v2 = (size_t)i2 * 3;

            if (v0 + 2 >= raw.vertices.size() || v1 + 2 >= raw.vertices.size() || v2 + 2 >= raw.vertices.size())
            {
                continue;
            }

            glm::vec3 a = {raw.vertices[v0], raw.vertices[v0 + 1], raw.vertices[v0 + 2]};
            glm::vec3 b = {raw.vertices[v1], raw.vertices[v1 + 1], raw.vertices[v1 + 2]};
            glm::vec3 c = {raw.vertices[v2], raw.vertices[v2 + 1], raw.vertices[v2 + 2]};

            a = glm::vec3(inst.localTransform * glm::vec4(a, 1.0f)) * scale;
            b = glm::vec3(inst.localTransform * glm::vec4(b, 1.0f)) * scale;
            c = glm::vec3(inst.localTransform * glm::vec4(c, 1.0f)) * scale;

            outTriangles.push_back({a, b, c});
        }
    }
    CH_CORE_INFO("Physics: Built mesh collider from '{}' ({} triangles).", modelPath, outTriangles.size());
}

void Physics::Update(Scene* scene, Timestep deltaTime, bool runtime)
{
    if (!runtime)
    {
        return;
    }

    PhysicsContext& ctx = GetContext(scene);
    ctx.Accumulator += deltaTime;

    const float kFixedDt = 1.0f / 60.0f;
    bool stepped = false;

    auto world = GetWorld();
    if (!world)
    {
        return;
    }

    while (ctx.Accumulator >= kFixedDt)
    {
        world->Step(kFixedDt);
        ctx.Accumulator -= kFixedDt;
        std::printf("Accumulator step passed\n");
        stepped = true;
    }

    if (stepped)
    {
        UpdateColliders(scene);
    }
}

RaycastResult Physics::Raycast(Scene* scene, Ray ray)
{
    if (auto world = GetWorld())
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
    auto world = GetWorld();
    if (world)
    {
        auto& registry = scene->GetRegistry();
        auto view = registry.view<RigidBodyComponent>();
        for (auto entity : view)
        {
            auto& rb = view.get<RigidBodyComponent>(entity);
            if (rb.Handle != kInvalidPhysicsBody)
            {
                world->DestroyBody(rb.Handle);
                rb.Handle = kInvalidPhysicsBody;
            }
        }
    }
    scene->GetRegistry().ctx().erase<PhysicsContext>();
}

void Physics::SetCollisionCallback(Scene* scene, std::function<void(entt::entity, entt::entity)> callback)
{
    GetContext(scene).CollisionCallback = callback;
}

void Physics::UpdateColliders(Scene* scene)
{
    auto world = GetWorld();
    auto& registry = scene->GetRegistry();
    auto view = registry.view<TransformComponent, RigidBodyComponent>();

    for (auto entity : view)
    {
        auto& transform = view.get<TransformComponent>(entity);
        auto& rb = view.get<RigidBodyComponent>(entity);

        if (rb.Handle == kInvalidPhysicsBody)
        {
            continue;
        }
        if (rb.Type == RigidBodyComponent::BodyType::Static)
        {
            continue;
        }

        glm::vec3 pos;
        glm::quat rot;
        world->GetTransform(rb.Handle, pos, rot);

        transform.Translation = pos;
        transform.RotationQuat = rot;
        transform.Rotation = glm::eulerAngles(rot);
        transform.IsDirty = true;
    }
}

void Physics::ApplyAutoCalculate(entt::entity entity, entt::registry& registry, ColliderComponent& collider,
                                  const glm::vec3& scale)
{
    // Determine which model to use: prefer collider's own ModelPath, fall back to ModelComponent
    std::string modelPath = collider.ModelPath;
    if (modelPath.empty())
    {
        if (auto* mc = registry.try_get<ModelComponent>(entity))
        {
            modelPath = mc->ModelPath;
        }
    }

    if (modelPath.empty())
    {
        CH_CORE_WARN("Physics::ApplyAutoCalculate: no model path found for entity={}", (uint32_t)entity);
        return;
    }

    // ── Build raw triangles (same path as mesh collider) ─────────────────────
    std::vector<PhysicsTriangle> triangles;
    BuildMeshTriangles(modelPath, scale, triangles);

    if (triangles.empty())
    {
        CH_CORE_WARN("Physics::ApplyAutoCalculate: model '{}' produced no triangles — skipping.", modelPath);
        return;
    }

    // ── Compute AABB directly from triangles (no MeshShape needed) ────────────
    glm::vec3 bMin(std::numeric_limits<float>::max());
    glm::vec3 bMax(std::numeric_limits<float>::lowest());
    for (const auto& t : triangles)
    {
        bMin = glm::min(bMin, glm::min(t.V0, glm::min(t.V1, t.V2)));
        bMax = glm::max(bMax, glm::max(t.V0, glm::max(t.V1, t.V2)));
    }

    // ── Convert Jolt AABB → collider fields ──────────────────────────────────
    // BuildMeshTriangles already multiplied vertices by `scale`, so the AABB is
    // in SCALED space.  However, InitializeBodies will scale the values again:
    //   Box:    desc.Dimensions = collider->Size * transform.Scale * 0.5f
    //   Sphere: desc.Dimensions.x = collider->Radius * compMax(scale)
    //   Mesh:   desc.Offset = collider->Offset * transform.Scale
    // To avoid double-scaling we divide back by scale before storing.
    glm::vec3 safeScale = glm::max(scale, glm::vec3(1e-5f));

    // Unscaled (local-space) values:
    glm::vec3 unscaledSize   = (bMax - bMin) / safeScale;
    glm::vec3 unscaledCenter = ((bMax + bMin) * 0.5f) / safeScale;

    collider.Size   = unscaledSize;
    collider.Radius = glm::compMax(unscaledSize) * 0.5f;
    collider.Height = unscaledSize.y;

    // For Mesh colliders do NOT touch Offset — the triangle data already encodes
    // the full geometry in local space; setting Offset here would double-shift it.
    if (collider.Type != ColliderType::Mesh)
    {
        collider.Offset = unscaledCenter;
    }

    CH_CORE_INFO("Physics::ApplyAutoCalculate: entity={} model='{}' → Size=({:.2f},{:.2f},{:.2f}) "
                 "Offset=({:.2f},{:.2f},{:.2f}) Radius={:.2f} Height={:.2f}",
                 (uint32_t)entity, modelPath,
                 collider.Size.x, collider.Size.y, collider.Size.z,
                 collider.Offset.x, collider.Offset.y, collider.Offset.z,
                 collider.Radius, collider.Height);
}

} // namespace Chained
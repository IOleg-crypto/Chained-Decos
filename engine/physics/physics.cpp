#include "physics.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/model_asset.h"
#include "engine/common/thread_pool.h"
#include "engine/core/log.h"
#include "engine/core/service_locator.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "engine/scene/systems/scene_resource_manager.h"
#include "iphysics_world.h"
#include "jolt_physics_world.h"

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/RegisterTypes.h>

#include "engine/project/project.h"

#include <memory>
#include <vector>

namespace Chained
{

static std::unique_ptr<JPH::Factory> s_JoltFactory;

Physics::Physics() = default;
Physics::~Physics() = default;

void Physics::Initialize()
{
    if (s_JoltFactory)
        return;
    JPH::RegisterDefaultAllocator();
    s_JoltFactory = std::make_unique<JPH::Factory>();
    JPH::Factory::sInstance = s_JoltFactory.get();
    JPH::RegisterTypes();
    CH_CORE_INFO("Physics initialized (Jolt backend).");
}

void Physics::Shutdown()
{
    m_World.reset();
    s_JoltFactory.reset();
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

void Physics::ResetWorld(Scene* scene)
{
    if (m_World)
    {
        static_cast<JoltPhysicsWorld*>(m_World.get())->ClearShapeCache();
    }

    // Invalidate all rigid body handles before destroying the world
    if (scene)
    {
        auto& registry = scene->GetRegistry();
        auto view = registry.view<RigidBodyComponent>();
        for (auto entity : view)
        {
            auto& rb = view.get<RigidBodyComponent>(entity);
            rb.Handle = kInvalidPhysicsBody;
        }
        registry.ctx().erase<IPhysicsWorld*>();
    }

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
    if (!registry.ctx().contains<IPhysicsWorld*>())
    {
        registry.ctx().emplace<IPhysicsWorld*>(world);
    }

    SceneResources::BatchInitializeBodies(registry, world);

    CH_CORE_INFO("Physics::InitializeBodies — bodies initialized for scene '{}'.", scene->GetSettings().Name);
}

void Physics::Update(Scene* scene, Timestep deltaTime, bool runtime)
{
    if (!runtime)
    {
        return;
    }

    PhysicsContext& ctx = GetContext(scene);
    ctx.Accumulator += deltaTime;

    float kFixedDt = 1.0f / 60.0f;
    if (auto project = Project::GetActive())
    {
        kFixedDt = project->GetConfig().Physics.FixedTimestep;
    }
    const int kMaxStepsPerFrame = 8;
    bool stepped = false;

    auto world = GetWorld();
    if (!world)
    {
        return;
    }

    auto& registry = scene->GetRegistry();
    auto view = registry.view<TransformComponent, RigidBodyComponent>();

    for (auto entity : view)
    {
        auto& rb = view.get<RigidBodyComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);

        if (rb.Handle == kInvalidPhysicsBody)
        {
            continue;
        }

        if (rb.Type == RigidBodyComponent::BodyType::Dynamic)
        {
            glm::vec3 currentJoltVelocity = world->GetVelocity(rb.Handle);
            glm::vec3 finalVelocity = rb.Velocity;
            if (rb.Velocity.y <= 0.5f)
            {
                finalVelocity.y = currentJoltVelocity.y;
            }
            world->SetVelocity(rb.Handle, finalVelocity);
        }
        else if (rb.Type == RigidBodyComponent::BodyType::Kinematic)
        {
            world->SetTransform(rb.Handle, transform.Translation, transform.RotationQuat);
            world->SetVelocity(rb.Handle, rb.Velocity);
            transform.IsDirty = false;
            continue;
        }

        if (transform.IsDirty)
        {
            world->SetTransform(rb.Handle, transform.Translation, transform.RotationQuat);
            transform.IsDirty = false;
        }
    }

    int steps = 0;
    while (ctx.Accumulator >= kFixedDt && steps < kMaxStepsPerFrame)
    {
        world->ClearGroundedState();
        world->Step(kFixedDt);
        ctx.Accumulator -= kFixedDt;
        stepped = true;
        steps++;
    }

    if (ctx.Accumulator >= kFixedDt)
    {
        ctx.Accumulator = 0.0f;
    }


    if (stepped)
    {
        UpdateColliders(scene);
    }
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

        bool isActive = world->IsBodyActive(rb.Handle);

        if (rb.Type == RigidBodyComponent::BodyType::Kinematic)
        {
            // Kinematic: position is controlled by script, but IsGrounded is still needed.
            // Only update IsGrounded for active bodies; sleeping bodies do not move,
            // so their grounded state remains correct from the previous frame.
            if (isActive)
                rb.IsGrounded = world->IsBodyGrounded(rb.Handle);
            continue;
        }

        // Dynamic: read position, velocity and grounded state from Jolt
        // For sleeping bodies - position hasn't changed, IsGrounded is kept from previous frame.
        if (!isActive)
            continue;

        glm::vec3 pos;
        glm::quat rot;
        world->GetTransform(rb.Handle, pos, rot);

        transform.Translation = pos;
        transform.RotationQuat = rot;
        transform.Rotation = glm::eulerAngles(rot);
        transform.IsDirty = true;

        rb.Velocity = world->GetVelocity(rb.Handle);
        rb.IsGrounded = world->IsBodyGrounded(rb.Handle);
    }
}

RaycastResult Physics::Raycast(Scene* scene, Ray ray)
{
    if (auto world = GetWorld())
    {
        return world->Raycast(ray.position, ray.direction, 1600.0f);
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
    auto& registry = scene->GetRegistry();

    if (m_World)
    {
        auto view = registry.view<RigidBodyComponent>();
        for (auto entity : view)
        {
            auto& rb = view.get<RigidBodyComponent>(entity);
            if (rb.Handle != kInvalidPhysicsBody)
            {
                m_World->DestroyBody(rb.Handle);
                rb.Handle = kInvalidPhysicsBody;
            }
        }
    }
    registry.ctx().erase<PhysicsContext>();
}

void Physics::SetCollisionCallback(Scene* scene, std::function<void(entt::entity, entt::entity)> callback)
{
    GetContext(scene).CollisionCallback = callback;
}

void Physics::ApplyAutoCalculate(entt::entity entity, entt::registry& registry, ColliderComponent& collider,
                                 const glm::vec3& scale)
{
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

    auto* am = ServiceLocator::Get<AssetManager>();
    auto handle = am->ResolveToHandle(modelPath);
    if (handle == AssetHandle(0))
    {
        CH_CORE_WARN("Physics::ApplyAutoCalculate: model '{}' not loaded.", modelPath);
        return;
    }

    auto asset = am->Get<ModelAsset>(handle);
    if (!asset || asset->GetState() != AssetState::Ready)
    {
        CH_CORE_WARN("Physics::ApplyAutoCalculate: model '{}' not ready.", modelPath);
        return;
    }

    const auto& bbox = asset->GetBoundingBox();
    glm::vec3 bMin = bbox.Min * scale;
    glm::vec3 bMax = bbox.Max * scale;

    glm::vec3 size = bMax - bMin;
    glm::vec3 center = (bMax + bMin) * 0.5f;

    collider.Size = size;
    collider.Radius = glm::compMax(size) * 0.5f;
    collider.Height = size.y;

    if (collider.Type != ColliderType::Mesh)
    {
        collider.Offset = center;
    }

    CH_CORE_INFO("Physics::ApplyAutoCalculate: entity={} model='{}' → Size=({:.2f},{:.2f},{:.2f}) "
                 "Offset=({:.2f},{:.2f},{:.2f}) Radius={:.2f} Height={:.2f}",
                 (uint32_t)entity, modelPath, collider.Size.x, collider.Size.y, collider.Size.z, collider.Offset.x,
                 collider.Offset.y, collider.Offset.z, collider.Radius, collider.Height);
}

} // namespace Chained
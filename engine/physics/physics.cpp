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

#include <future>
#include <mutex>
#include <vector>

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
    if (m_World)
    {
        static_cast<JoltPhysicsWorld*>(m_World.get())->ClearShapeCache();
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

    // Make sure the IPhysicsWorld* is in the registry context so that
    // SceneResourceManager::Update() can detect pending bodies later too.
    auto& registry = scene->GetRegistry();
    if (!registry.ctx().contains<IPhysicsWorld*>())
    {
        registry.ctx().emplace<IPhysicsWorld*>(world);
    }

    // Iterate all entities that need a physics body but don't have one yet.
    // The actual body creation logic lives in SceneResourceManager::OnRigidBodyConstruct;
    // triggering it here ensures bodies are ready before the first script OnCreate/OnUpdate.
    auto* resourceManager = scene->GetResourceManager();
    if (!resourceManager)
    {
        return;
    }

    auto view = registry.view<RigidBodyComponent, TransformComponent>();
    for (auto entity : view)
    {
        auto& rb = view.get<RigidBodyComponent>(entity);
        if (rb.Handle == kInvalidPhysicsBody)
        {
            resourceManager->OnRigidBodyConstruct(registry, entity);
        }
    }

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

    // ── КРОК 1: Синхронізація швидкостей та телепортів з ECS у Jolt перед симуляцією ──
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
            // Jolt є єдиним авторитетом для Y (гравітація).
            // Скрипт контролює X, Z. Для стрибка — rb.Velocity.y > 0.5 (явний імпульс).
            glm::vec3 currentJoltVelocity = world->GetVelocity(rb.Handle);
            glm::vec3 finalVelocity = rb.Velocity;
            // Завжди берємо Y з Jolt, окрім стрибкового імпульсу зі скрипту
            if (rb.Velocity.y <= 0.5f)
            {
                finalVelocity.y = currentJoltVelocity.y;
            }
            world->SetVelocity(rb.Handle, finalVelocity);
        }
        else if (rb.Type == RigidBodyComponent::BodyType::Kinematic)
        {
            // Кінематичне тіло: скрипт рухає transform напряму.
            // Потрібно синхронізувати позицію в Jolt, щоб контакти спрацювали.
            world->SetTransform(rb.Handle, transform.Translation, transform.RotationQuat);
            world->SetVelocity(rb.Handle, rb.Velocity);
            transform.IsDirty = false;
            continue; // позиція вже синхронізована, skip загального IsDirty нижче
        }

        // Для Dynamic: обробляємо телепортацію (IsDirty з редактора/коду)
        if (transform.IsDirty)
        {
            world->SetTransform(rb.Handle, transform.Translation, transform.RotationQuat);
            transform.IsDirty = false;
        }
    }

    // ── КРОК 2: Фізичний крок симуляції Jolt ──
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
        ctx.Accumulator = 0.0f; // Захист від накопичення затримок (зависання дебагера тощо)
    }

    // ── КРОК 3: Оновлюємо компоненти за результатами повної симуляції кадрів ──
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

        if (rb.Type == RigidBodyComponent::BodyType::Kinematic)
        {
            // Кінематика: позиція контролюється скриптом, але IsGrounded потрібен.
            rb.IsGrounded = world->IsBodyGrounded(rb.Handle);
            continue;
        }

        // Dynamic: читаємо позицію, швидкість та стан заземлення з Jolt
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
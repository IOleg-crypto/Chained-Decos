#include "jolt_physics_world.h"
#include "engine/core/log.h"

#include <Jolt/Core/Factory.h>
#include <Jolt/Geometry/Triangle.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/RegisterTypes.h>

#include <chrono>

namespace Chained
{

// ─────────────────────────────────────────────────────────────────────────────
// Layer setup — two layers: NON_MOVING (static) and MOVING (dynamic/kinematic)
// ─────────────────────────────────────────────────────────────────────────────
namespace Layers
{
static constexpr JPH::ObjectLayer NON_MOVING = 0;
static constexpr JPH::ObjectLayer MOVING = 1;
static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
} // namespace Layers

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override
    {
        switch (a)
        {
        case Layers::NON_MOVING:
            return b == Layers::MOVING;
        case Layers::MOVING:
            return true;
        default:
            return false;
        }
    }
};

namespace BroadPhaseLayers
{
static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
static constexpr JPH::BroadPhaseLayer MOVING(1);
static constexpr uint32_t NUM_LAYERS = 2;
} // namespace BroadPhaseLayers

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        m_Map[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        m_Map[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }
    uint32_t GetNumBroadPhaseLayers() const override
    {
        return BroadPhaseLayers::NUM_LAYERS;
    }
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
    {
        return m_Map[layer];
    }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
    {
        return (JPH::BroadPhaseLayer::Type)layer == 0 ? "NON_MOVING" : "MOVING";
    }
#endif
private:
    JPH::BroadPhaseLayer m_Map[Layers::NUM_LAYERS];
};

class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer bpLayer) const override
    {
        switch (layer)
        {
        case Layers::NON_MOVING:
            return bpLayer == BroadPhaseLayers::MOVING;
        case Layers::MOVING:
            return true;
        default:
            return false;
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// File-scope singletons — survive JoltPhysicsWorld::ResetWorld() rebuilds
// ─────────────────────────────────────────────────────────────────────────────
static BPLayerInterfaceImpl s_BPLayerInterface;
static ObjectVsBroadPhaseLayerFilterImpl s_ObjVsBPFilter;
static ObjectLayerPairFilterImpl s_ObjVsObjFilter;

// ─────────────────────────────────────────────────────────────────────────────
// JoltPhysicsWorld
// ─────────────────────────────────────────────────────────────────────────────
JoltPhysicsWorld::JoltPhysicsWorld()
{
    m_TempAllocator = new JPH::TempAllocatorImpl(32 * 1024 * 1024);
    m_JobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
                                               (int)std::thread::hardware_concurrency() - 1);

    m_PhysicsSystem.Init(65536, 0, 65536, 65536, s_BPLayerInterface, s_ObjVsBPFilter, s_ObjVsObjFilter);

    m_PhysicsSystem.SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

    m_ContactListener.SetGroundedTracker(&m_GroundedBodies);
    m_PhysicsSystem.SetContactListener(&m_ContactListener);

    CH_CORE_INFO("Jolt Physics World Initialized with ContactListener.");
}

JoltPhysicsWorld::~JoltPhysicsWorld()
{
    m_MeshShapeCache.clear();
    delete m_JobSystem;
    delete m_TempAllocator;
}

void JoltPhysicsWorld::ClearShapeCache()
{
    m_MeshShapeCache.clear();
}

PhysicsBodyHandle JoltPhysicsWorld::CreateBody(const PhysicsBodyDesc& desc)
{
    JPH::BodyInterface& bi = m_PhysicsSystem.GetBodyInterface();

    // ── 1. Build shape ───────────────────────────────────────────────────────
    JPH::ShapeRefC shape;

    switch (desc.Shape)
    {
    case ColliderType::Box: {
        JPH::BoxShapeSettings s(JPH::Vec3(desc.Dimensions.x, desc.Dimensions.y, desc.Dimensions.z));
        shape = s.Create().Get();
        break;
    }
    case ColliderType::Sphere: {
        JPH::SphereShapeSettings s(desc.Dimensions.x);
        shape = s.Create().Get();
        break;
    }
    case ColliderType::Capsule: {
        JPH::CapsuleShapeSettings s(desc.Dimensions.y, desc.Dimensions.x);
        shape = s.Create().Get();
        break;
    }
    case ColliderType::Mesh: {
        if (desc.Triangles.empty())
        {
            CH_CORE_WARN("Physics: MeshShape requested but no triangles provided — falling back to unit box.");
            shape = JPH::BoxShapeSettings(JPH::Vec3(0.5f, 0.5f, 0.5f)).Create().Get();
            break;
        }

        // The cache holds the bare, unscaled BVH keyed by model path. Scale (ScaledShape)
        // and offset (RotatedTranslatedShape) are layered on afterwards as cheap decorators,
        // so every instance of the same model — regardless of scale or offset — reuses one BVH.
        JPH::ShapeRefC baseShape;
        bool cached = false;
        if (!desc.CacheKey.empty())
        {
            auto it = m_MeshShapeCache.find(desc.CacheKey);
            if (it != m_MeshShapeCache.end())
            {
                baseShape = it->second;
                cached = true;
            }
        }

        if (!cached)
        {
            const auto buildStart = std::chrono::steady_clock::now();
            JPH::TriangleList joltTris;
            joltTris.reserve(desc.Triangles.size());
            for (const auto& t : desc.Triangles)
            {
                JPH::Triangle tri(JPH::Float3(t.V0.x, t.V0.y, t.V0.z), JPH::Float3(t.V1.x, t.V1.y, t.V1.z),
                                  JPH::Float3(t.V2.x, t.V2.y, t.V2.z));
                JPH::Vec3 v0 = JPH::Vec3::sLoadFloat3Unsafe(tri.mV[0]);
                JPH::Vec3 v1 = JPH::Vec3::sLoadFloat3Unsafe(tri.mV[1]);
                JPH::Vec3 v2 = JPH::Vec3::sLoadFloat3Unsafe(tri.mV[2]);
                JPH::Vec3 e1 = v1 - v0;
                JPH::Vec3 e2 = v2 - v0;
                if (e1.Cross(e2).LengthSq() < 1e-20f)
                {
                    continue;
                }
                joltTris.push_back(tri);
            }

            if (joltTris.empty())
            {
                CH_CORE_WARN("Physics: All mesh triangles degenerate — falling back to unit box.");
                shape = JPH::BoxShapeSettings(JPH::Vec3(0.5f, 0.5f, 0.5f)).Create().Get();
                break;
            }

            JPH::MeshShapeSettings s(std::move(joltTris));
            s.mBuildQuality = JPH::MeshShapeSettings::EBuildQuality::FavorBuildSpeed;
            auto result = s.Create();
            if (result.HasError())
            {
                CH_CORE_ERROR("Physics: MeshShape build failed: {} — falling back to unit box.",
                              result.GetError().c_str());
                shape = JPH::BoxShapeSettings(JPH::Vec3(0.5f, 0.5f, 0.5f)).Create().Get();
                break;
            }

            baseShape = result.Get();
            if (!desc.CacheKey.empty())
            {
                m_MeshShapeCache[desc.CacheKey] = baseShape;
            }

            const auto buildEnd = std::chrono::steady_clock::now();
            const double buildMs =
                std::chrono::duration<double, std::milli>(buildEnd - buildStart).count();
            CH_CORE_INFO("Physics: Built mesh BVH ({} tris) in {:.2f} ms{} [key='{}']", joltTris.size(), buildMs,
                         desc.CacheKey.empty() ? " (uncached)" : " (cached)", desc.CacheKey);
        }

        // Apply per-instance scale as a decorator (identity scale needs no wrapper).
        shape = baseShape;
        if (desc.MeshScale != glm::vec3(1.0f))
        {
            JPH::ScaledShapeSettings scaledSettings(
                baseShape, JPH::Vec3(desc.MeshScale.x, desc.MeshScale.y, desc.MeshScale.z));
            auto scaledResult = scaledSettings.Create();
            if (!scaledResult.HasError())
            {
                shape = scaledResult.Get();
            }
            else
            {
                CH_CORE_WARN("Physics: ScaledShape build failed: {} — using unscaled mesh.",
                             scaledResult.GetError().c_str());
            }
        }
        break;
    }
    default: {
        CH_CORE_WARN("Physics: Unknown ColliderType — falling back to unit box.");
        shape = JPH::BoxShapeSettings(JPH::Vec3(0.5f, 0.5f, 0.5f)).Create().Get();
        break;
    }
    }

    // ── 1b. Apply Offset if present ──────────────────────────────────────────
    if (desc.Offset != glm::vec3(0.0f))
    {
        JPH::RotatedTranslatedShapeSettings offsetSettings(JPH::Vec3(desc.Offset.x, desc.Offset.y, desc.Offset.z),
                                                           JPH::Quat::sIdentity(), shape);
        auto offsetResult = offsetSettings.Create();
        if (!offsetResult.HasError())
        {
            shape = offsetResult.Get();
        }
    }

    // ── 2. Determine motion type ─────────────────────────────────────────────
    JPH::EMotionType motionType;
    JPH::ObjectLayer objectLayer;

    if (desc.IsStatic)
    {
        motionType = JPH::EMotionType::Static;
        objectLayer = Layers::NON_MOVING;
    }
    else if (desc.IsKinematic)
    {
        motionType = JPH::EMotionType::Kinematic;
        objectLayer = Layers::MOVING;
    }
    else
    {
        motionType = JPH::EMotionType::Dynamic;
        objectLayer = Layers::MOVING;
    }

    // ── 3. Create body ───────────────────────────────────────────────────────
    JPH::BodyCreationSettings settings(shape, JPH::RVec3(desc.Position.x, desc.Position.y, desc.Position.z),
                                       JPH::Quat(desc.Rotation.x, desc.Rotation.y, desc.Rotation.z, desc.Rotation.w),
                                       motionType, objectLayer);

    settings.mFriction = desc.Friction;
    settings.mRestitution = desc.Restitution;
    settings.mLinearDamping = desc.LinearDamping;
    settings.mAngularDamping = desc.AngularDamping;
    settings.mAllowSleeping = true;

    if (desc.IsKinematic)
    {
        settings.mGravityFactor = 0.0f;
    }
    else
    {
        settings.mGravityFactor = desc.UseGravity ? 1.0f : 0.0f;
    }

    if (desc.IsFixedRotation)
    {
        settings.mAllowedDOFs =
            JPH::EAllowedDOFs::TranslationX | JPH::EAllowedDOFs::TranslationY | JPH::EAllowedDOFs::TranslationZ;
    }

    settings.mUserData = desc.UserData;

    JPH::Body* body = bi.CreateBody(settings);
    bi.AddBody(body->GetID(), JPH::EActivation::Activate);

    return (PhysicsBodyHandle)body->GetID().GetIndexAndSequenceNumber();
}

void JoltPhysicsWorld::DestroyBody(PhysicsBodyHandle handle)
{
    JPH::BodyInterface& bi = m_PhysicsSystem.GetBodyInterface();
    JPH::BodyID id((JPH::uint32)handle);
    bi.RemoveBody(id);
    bi.DestroyBody(id);
}

void JoltPhysicsWorld::SetTransform(PhysicsBodyHandle handle, const glm::vec3& pos, const glm::quat& rot)
{
    m_PhysicsSystem.GetBodyInterface().SetPositionAndRotation((JPH::BodyID)handle, JPH::RVec3(pos.x, pos.y, pos.z),
                                                              JPH::Quat(rot.x, rot.y, rot.z, rot.w),
                                                              JPH::EActivation::Activate);
}

void JoltPhysicsWorld::GetTransform(PhysicsBodyHandle handle, glm::vec3& pos, glm::quat& rot)
{
    JPH::RVec3 jPos;
    JPH::Quat jRot;
    m_PhysicsSystem.GetBodyInterface().GetPositionAndRotation((JPH::BodyID)handle, jPos, jRot);
    pos = {jPos.GetX(), jPos.GetY(), jPos.GetZ()};
    rot = {jRot.GetW(), jRot.GetX(), jRot.GetY(), jRot.GetZ()};
}

void JoltPhysicsWorld::SetVelocity(PhysicsBodyHandle handle, const glm::vec3& velocity)
{
    m_PhysicsSystem.GetBodyInterface().SetLinearVelocity((JPH::BodyID)handle,
                                                         JPH::Vec3(velocity.x, velocity.y, velocity.z));
}

glm::vec3 JoltPhysicsWorld::GetVelocity(PhysicsBodyHandle handle) const
{
    JPH::Vec3 v = m_PhysicsSystem.GetBodyInterface().GetLinearVelocity((JPH::BodyID)handle);
    return {v.GetX(), v.GetY(), v.GetZ()};
}

RaycastResult JoltPhysicsWorld::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance)
{
    JPH::RRayCast ray{JPH::RVec3(origin.x, origin.y, origin.z),
                      JPH::Vec3(direction.x, direction.y, direction.z) * maxDistance};

    JPH::RayCastResult result;
    if (m_PhysicsSystem.GetNarrowPhaseQuery().CastRay(ray, result))
    {
        JPH::RVec3 hitPos = ray.GetPointOnRay(result.mFraction);
        RaycastResult out;
        out.Hit = true;
        out.Distance = result.mFraction * maxDistance;
        out.Position = {hitPos.GetX(), hitPos.GetY(), hitPos.GetZ()};
        out.BodyHandle = (PhysicsBodyHandle)result.mBodyID.GetIndexAndSequenceNumber();
        out.Entity = (entt::entity)m_PhysicsSystem.GetBodyInterface().GetUserData(result.mBodyID);
        return out;
    }
    return RaycastResult{false};
}

void JoltPhysicsWorld::Step(float fixedDt)
{
    m_PhysicsSystem.Update(fixedDt, 1, m_TempAllocator, m_JobSystem);
}

void JoltPhysicsWorld::SetGravity(float gravity)
{
    m_PhysicsSystem.SetGravity(JPH::Vec3(0.0f, -gravity, 0.0f));
}

// ─────────────────────────────────────────────────────────────────────────────
// Ground detection via contact callbacks (O(1) lookup)
// ─────────────────────────────────────────────────────────────────────────────
bool JoltPhysicsWorld::IsBodyGrounded(PhysicsBodyHandle handle) const
{
    if (handle == 0)
        return false;
    return m_GroundedBodies.count(static_cast<uint32_t>(handle)) > 0;
}

void JoltPhysicsWorld::ClearGroundedState()
{
    m_GroundedBodies.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// ContactListenerImpl
// ─────────────────────────────────────────────────────────────────────────────
JPH::ValidateResult JoltPhysicsWorld::ContactListenerImpl::OnContactValidate(
    const JPH::Body&, const JPH::Body&, JPH::RVec3Arg, const JPH::CollideShapeResult&)
{
    return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
}

static void RegisterGroundContact(std::unordered_set<uint32_t>* tracker,
                                   const JPH::Body& body1, const JPH::Body& body2,
                                   const JPH::ContactManifold& manifold)
{
    if (!tracker) return;
    // Normal points from body2 to body1; upward contact means body1 is on top of body2.
    JPH::Vec3 normal = manifold.mWorldSpaceNormal;
    // If the normal Y > 0.5 → body2 is the ground under body1
    if (normal.GetY() > 0.5f)
        tracker->insert(body1.GetID().GetIndexAndSequenceNumber());
    // If normal Y < -0.5 → body1 is the ground under body2
    else if (normal.GetY() < -0.5f)
        tracker->insert(body2.GetID().GetIndexAndSequenceNumber());
}

void JoltPhysicsWorld::ContactListenerImpl::OnContactAdded(
    const JPH::Body& body1, const JPH::Body& body2,
    const JPH::ContactManifold& manifold, JPH::ContactSettings&)
{
    RegisterGroundContact(m_Tracker, body1, body2, manifold);
}

void JoltPhysicsWorld::ContactListenerImpl::OnContactPersisted(
    const JPH::Body& body1, const JPH::Body& body2,
    const JPH::ContactManifold& manifold, JPH::ContactSettings&)
{
    RegisterGroundContact(m_Tracker, body1, body2, manifold);
}

void JoltPhysicsWorld::ContactListenerImpl::OnContactRemoved(const JPH::SubShapeIDPair&)
{
    // Grounded set is rebuilt each frame by ClearGroundedState() + step contacts.
}

} // namespace Chained
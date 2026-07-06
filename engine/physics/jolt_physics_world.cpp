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
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/RegisterTypes.h>
#include <thread>

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

class ContactListenerImpl : public JPH::ContactListener
{
public:
    JPH::ValidateResult OnContactValidate(const JPH::Body&, const JPH::Body&, JPH::RVec3Arg,
                                          const JPH::CollideShapeResult&) override
    {
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }
    void OnContactAdded(const JPH::Body&, const JPH::Body&, const JPH::ContactManifold&, JPH::ContactSettings&) override
    {
    }
    void OnContactPersisted(const JPH::Body&, const JPH::Body&, const JPH::ContactManifold&,
                            JPH::ContactSettings&) override
    {
    }
    void OnContactRemoved(const JPH::SubShapeIDPair&) override
    {
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// File-scope singletons — survive JoltPhysicsWorld::ResetWorld() rebuilds
// ─────────────────────────────────────────────────────────────────────────────
static BPLayerInterfaceImpl s_BPLayerInterface;
static ObjectVsBroadPhaseLayerFilterImpl s_ObjVsBPFilter;
static ObjectLayerPairFilterImpl s_ObjVsObjFilter;
static ContactListenerImpl s_ContactListener;

// ─────────────────────────────────────────────────────────────────────────────
// JoltPhysicsWorld
// ─────────────────────────────────────────────────────────────────────────────
JoltPhysicsWorld::JoltPhysicsWorld()
{
    m_TempAllocator = new JPH::TempAllocatorImpl(32 * 1024 * 1024);
    m_JobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
                                               (int)std::thread::hardware_concurrency() - 1);

    // Use file-scope singletons so they survive world recreation
    m_PhysicsSystem.Init(65536, 0, 65536, 65536, s_BPLayerInterface, s_ObjVsBPFilter, s_ObjVsObjFilter);
    m_PhysicsSystem.SetContactListener(&s_ContactListener);

    // Set gravity
    m_PhysicsSystem.SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

    CH_CORE_INFO("Jolt Physics World Initialized.");
}

JoltPhysicsWorld::~JoltPhysicsWorld()
{
    delete m_JobSystem;
    delete m_TempAllocator;
}

PhysicsBodyHandle JoltPhysicsWorld::CreateBody(const PhysicsBodyDesc& desc)
{
    JPH::BodyInterface& bi = m_PhysicsSystem.GetBodyInterface();

    // ── 1. Build shape ───────────────────────────────────────────────────────
    JPH::ShapeRefC shape;

    switch (desc.Shape)
    {
    case ColliderType::Box: {
        // Dimensions = half-extents
        JPH::BoxShapeSettings s(JPH::Vec3(desc.Dimensions.x, desc.Dimensions.y, desc.Dimensions.z));
        shape = s.Create().Get();
        break;
    }
    case ColliderType::Sphere: {
        // Dimensions.x = radius
        JPH::SphereShapeSettings s(desc.Dimensions.x);
        shape = s.Create().Get();
        break;
    }
    case ColliderType::Capsule: {
        // Dimensions.x = radius, Dimensions.y = half-height
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
            CH_CORE_ERROR("Physics: MeshShape build failed: {} — falling back to unit box.", result.GetError().c_str());
            shape = JPH::BoxShapeSettings(JPH::Vec3(0.5f, 0.5f, 0.5f)).Create().Get();
        }
        else
        {
            shape = result.Get();
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
        JPH::RotatedTranslatedShapeSettings offsetSettings(
            JPH::Vec3(desc.Offset.x, desc.Offset.y, desc.Offset.z),
            JPH::Quat::sIdentity(),
            shape
        );
        auto offsetResult = offsetSettings.Create();
        if (!offsetResult.HasError())
        {
            shape = offsetResult.Get();
        }
    }

    // ── 2. Determine motion type ─────────────────────────────────────────────
    JPH::EMotionType motionType;
    JPH::ObjectLayer objectLayer;

    if (desc.IsStatic || desc.Shape == ColliderType::Mesh)
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
    settings.mGravityFactor = desc.UseGravity ? 1.0f : 0.0f;

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

} // namespace Chained
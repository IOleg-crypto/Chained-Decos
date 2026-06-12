#include "jolt_physics_world.h"
#include "engine/core/log.h"
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <thread>

namespace Chained
{

namespace Layers
{
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
    {
        switch (inObject1)
        {
        case Layers::NON_MOVING: return inObject2 == Layers::MOVING;
        case Layers::MOVING: return true;
        default: return false;
        }
    }
};

namespace BroadPhaseLayers
{
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr uint32_t NUM_LAYERS = 2;
};

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }

    virtual uint32_t GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }
    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override { return mObjectToBroadPhase[inLayer]; }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        switch ((JPH::BroadPhaseLayer::Type)inLayer)
        {
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING: return "MOVING";
        default: return "INVALID";
        }
    }
#endif
private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1)
        {
        case Layers::NON_MOVING: return inLayer2 == BroadPhaseLayers::MOVING;
        case Layers::MOVING: return true;
        default: return false;
        }
    }
};

class ContactListenerImpl : public JPH::ContactListener
{
public:
    virtual JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override
    {
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
    {
        // CH_CORE_TRACE("Collision Added!");
    }

    virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override {}
    virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override {}
};

JoltPhysicsWorld::JoltPhysicsWorld()
{
    CH_CORE_INFO("JoltPhysicsWorld::Constructor - Start");

    m_TempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
    CH_CORE_INFO("JoltPhysicsWorld::Constructor - Temp allocator created");
    
    m_JobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, (int)std::thread::hardware_concurrency() - 1);
    CH_CORE_INFO("JoltPhysicsWorld::Constructor - Job system created");

    const uint32_t cMaxBodies = 1024;
    const uint32_t cMaxBodyPairs = 1024;
    const uint32_t cMaxContactConstraints = 1024;

    static BPLayerInterfaceImpl bp_layer_interface;
    static ObjectVsBroadPhaseLayerFilterImpl obj_vs_bp_layer_filter;
    static ObjectLayerPairFilterImpl obj_vs_obj_layer_filter;

    CH_CORE_INFO("JoltPhysicsWorld::Constructor - Initializing PhysicsSystem");
    m_PhysicsSystem.Init(cMaxBodies, 0, cMaxBodyPairs, cMaxContactConstraints, bp_layer_interface, obj_vs_bp_layer_filter, obj_vs_obj_layer_filter);
    CH_CORE_INFO("JoltPhysicsWorld::Constructor - PhysicsSystem initialized");

    static ContactListenerImpl contact_listener;
    m_PhysicsSystem.SetContactListener(&contact_listener);

    CH_CORE_INFO("Jolt Physics World Initialized.");
}

JoltPhysicsWorld::~JoltPhysicsWorld()
{
    delete m_JobSystem;
    delete m_TempAllocator;
}

PhysicsBodyHandle JoltPhysicsWorld::CreateBody(const PhysicsBodyDesc& desc)
{
    JPH::BodyInterface& body_interface = m_PhysicsSystem.GetBodyInterface();
    JPH::ShapeRefC shape;

    switch (desc.Shape)
    {
    case Chained::ColliderType::Box:
    {
        JPH::BoxShapeSettings settings(JPH::Vec3(desc.Dimensions.x * 0.5f, desc.Dimensions.y * 0.5f, desc.Dimensions.z * 0.5f));
        shape = settings.Create().Get();
        break;
    }
    case Chained::ColliderType::Sphere:
    {
        JPH::SphereShapeSettings settings(desc.Dimensions.x); // radius
        shape = settings.Create().Get();
        break;
    }
    case Chained::ColliderType::Capsule:
    {
        JPH::CapsuleShapeSettings settings(desc.Dimensions.y * 0.5f, desc.Dimensions.x); // half-height, radius
        shape = settings.Create().Get();
        break;
    }
    default:
    {
        JPH::BoxShapeSettings settings(JPH::Vec3(0.5f, 0.5f, 0.5f));
        shape = settings.Create().Get();
        break;
    }
    }

    JPH::BodyCreationSettings body_settings(shape, JPH::RVec3(desc.Position.x, desc.Position.y, desc.Position.z), 
                                          JPH::Quat(desc.Rotation.x, desc.Rotation.y, desc.Rotation.z, desc.Rotation.w),
                                          desc.IsKinematic ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic,
                                          desc.IsKinematic ? Layers::NON_MOVING : Layers::MOVING);

    body_settings.mAllowSleeping = true;
    body_settings.mFriction = 0.5f;
    body_settings.mRestitution = 0.0f;

    JPH::Body* body = body_interface.CreateBody(body_settings);
    body_interface.AddBody(body->GetID(), JPH::EActivation::Activate);

    return (PhysicsBodyHandle)body->GetID().GetIndexAndSequenceNumber();
}

void JoltPhysicsWorld::DestroyBody(PhysicsBodyHandle handle)
{
    JPH::BodyInterface& body_interface = m_PhysicsSystem.GetBodyInterface();
    JPH::BodyID id((JPH::uint32)handle);
    body_interface.RemoveBody(id);
    body_interface.DestroyBody(id);
}

void JoltPhysicsWorld::SetTransform(PhysicsBodyHandle handle, const glm::vec3& pos, const glm::quat& rot)
{
    JPH::BodyInterface& body_interface = m_PhysicsSystem.GetBodyInterface();
    body_interface.SetPositionAndRotation((JPH::BodyID)handle, JPH::RVec3(pos.x, pos.y, pos.z), JPH::Quat(rot.x, rot.y, rot.z, rot.w), JPH::EActivation::Activate);
}

void JoltPhysicsWorld::GetTransform(PhysicsBodyHandle handle, glm::vec3& pos, glm::quat& rot)
{
    JPH::BodyInterface& body_interface = m_PhysicsSystem.GetBodyInterface();
    JPH::RVec3 jPos;
    JPH::Quat jRot;
    body_interface.GetPositionAndRotation((JPH::BodyID)handle, jPos, jRot);
    pos = {jPos.GetX(), jPos.GetY(), jPos.GetZ()};
    rot = {jRot.GetW(), jRot.GetX(), jRot.GetY(), jRot.GetZ()};
}

void JoltPhysicsWorld::SetVelocity(PhysicsBodyHandle handle, const glm::vec3& velocity)
{
    m_PhysicsSystem.GetBodyInterface().SetLinearVelocity((JPH::BodyID)handle, JPH::Vec3(velocity.x, velocity.y, velocity.z));
}

glm::vec3 JoltPhysicsWorld::GetVelocity(PhysicsBodyHandle handle) const
{
    JPH::Vec3 v = m_PhysicsSystem.GetBodyInterface().GetLinearVelocity((JPH::BodyID)handle);
    return {v.GetX(), v.GetY(), v.GetZ()};
}

RaycastResult JoltPhysicsWorld::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance)
{
    JPH::RVec3 jOrigin(origin.x, origin.y, origin.z);
    JPH::Vec3 jDirection(direction.x, direction.y, direction.z);
    JPH::RRayCast ray{ jOrigin, jDirection * maxDistance };
    JPH::RayCastResult result;

    if (m_PhysicsSystem.GetNarrowPhaseQuery().CastRay(ray, result))
    {
        JPH::BodyInterface& body_interface = m_PhysicsSystem.GetBodyInterface();
        JPH::RVec3 hitPos = ray.GetPointOnRay(result.mFraction);
        
        RaycastResult finalResult;
        finalResult.Hit = true;
        finalResult.Distance = result.mFraction * maxDistance;
        finalResult.Position = {hitPos.GetX(), hitPos.GetY(), hitPos.GetZ()};
        finalResult.BodyHandle = (PhysicsBodyHandle)result.mBodyID.GetIndexAndSequenceNumber();
        
        return finalResult;
    }

    return RaycastResult{false};
}

void JoltPhysicsWorld::Step(float fixedDt)
{
    m_PhysicsSystem.Update(fixedDt, 1, m_TempAllocator, m_JobSystem);
}

} // namespace Chained

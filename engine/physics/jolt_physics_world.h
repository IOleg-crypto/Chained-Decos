#ifndef CH_JOLT_PHYSICS_WORLD_H
#define CH_JOLT_PHYSICS_WORLD_H

#include "iphysics_world.h"
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace Chained
{

// Jolt Physics backend. Wraps JPH::PhysicsSystem and provides body management,
// raycasting, gravity control, and ground detection via contact callbacks.
class JoltPhysicsWorld : public IPhysicsWorld
{
public:
    JoltPhysicsWorld();
    virtual ~JoltPhysicsWorld() override;

    virtual PhysicsBodyHandle CreateBody(const PhysicsBodyDesc& desc) override;
    virtual void DestroyBody(PhysicsBodyHandle handle) override;

    virtual void SetTransform(PhysicsBodyHandle handle, const glm::vec3& pos, const glm::quat& rot) override;
    virtual void GetTransform(PhysicsBodyHandle handle, glm::vec3& pos, glm::quat& rot) override;

    virtual void SetVelocity(PhysicsBodyHandle handle, const glm::vec3& velocity) override;
    virtual glm::vec3 GetVelocity(PhysicsBodyHandle handle) const override;

    virtual RaycastResult Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) override;

    virtual void Step(float fixedDt) override;

    virtual void SetGravity(float gravity) override;

    virtual bool IsBodyGrounded(PhysicsBodyHandle handle) const override;
    virtual bool IsBodyActive(PhysicsBodyHandle handle) const override;

    /// Clear all grounded state (call before/after world reset).
    virtual void ClearGroundedState() override;

    virtual bool HasCachedMeshShape(const std::string& key) const override;

    /// Clear the cached mesh shapes (call on world reset).
    void ClearShapeCache();

private:
    // ── Jolt subsystems ──────────────────────────────────────────────────────
    JPH::PhysicsSystem m_PhysicsSystem;
    std::unique_ptr<JPH::TempAllocatorImpl> m_TempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> m_JobSystem;

    // ── Ground detection ──────────────────────────────────────────────────────
    // Set of Jolt body IDs (packed as uint32) that have at least one ground
    // contact — i.e. a contact whose normal Y component is positive (pointing
    // upward relative to the body being checked).
    std::unordered_set<uint32_t> m_GroundedBodies;

    // ── Mesh shape cache ─────────────────────────────────────────────────────
    // Built MeshShapes are cached per triangle fingerprint (model path + scale)
    // so that multiple bodies using the same mesh share a single BVH build.
    std::unordered_map<std::string, JPH::RefConst<JPH::Shape>> m_MeshShapeCache;

    // Contact listener that populates m_GroundedBodies.
    class ContactListenerImpl : public JPH::ContactListener
    {
    public:
        JPH::ValidateResult OnContactValidate(const JPH::Body&, const JPH::Body&,
                                              JPH::RVec3Arg,
                                              const JPH::CollideShapeResult&) override;
        void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2,
                            const JPH::ContactManifold& inManifold,
                            JPH::ContactSettings& inSettings) override;
        void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2,
                                const JPH::ContactManifold& inManifold,
                                JPH::ContactSettings& inSettings) override;
        void OnContactRemoved(const JPH::SubShapeIDPair& inPair) override;

        /// Set the tracker that receives ground-contact updates.
        void SetGroundedTracker(std::unordered_set<uint32_t>* tracker) { m_Tracker = tracker; }

    private:
        std::unordered_set<uint32_t>* m_Tracker = nullptr;
    };

    ContactListenerImpl m_ContactListener;
};

} // namespace Chained

#endif // CH_JOLT_PHYSICS_WORLD_H

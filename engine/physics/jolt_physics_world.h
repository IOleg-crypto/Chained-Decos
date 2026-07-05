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
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

namespace Chained
{

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

private:
    JPH::PhysicsSystem m_PhysicsSystem;
    JPH::TempAllocatorImpl* m_TempAllocator = nullptr;
    JPH::JobSystemThreadPool* m_JobSystem = nullptr;
};

} // namespace Chained

#endif // CH_JOLT_PHYSICS_WORLD_H

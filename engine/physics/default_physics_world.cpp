#include "default_physics_world.h"
#include "raycast_result.h"

namespace Chained
{

DefaultPhysicsWorld::DefaultPhysicsWorld()
{
}

PhysicsBodyHandle DefaultPhysicsWorld::CreateBody(const PhysicsBodyDesc& desc)
{
    PhysicsBodyHandle handle = m_NextHandle++;
    DefaultPhysicsBody body;
    body.Handle = handle;
    body.Position = desc.Position;
    body.Rotation = desc.Rotation;
    body.Velocity = desc.InitialVelocity;
    body.Mass = desc.Mass;
    body.IsKinematic = desc.IsKinematic;
    body.UseGravity = desc.UseGravity;
    
    m_Bodies[handle] = body;
    return handle;
}

void DefaultPhysicsWorld::DestroyBody(PhysicsBodyHandle handle)
{
    m_Bodies.erase(handle);
}

void DefaultPhysicsWorld::SetTransform(PhysicsBodyHandle handle, const glm::vec3& pos, const glm::quat& rot)
{
    if (auto it = m_Bodies.find(handle); it != m_Bodies.end())
    {
        it->second.Position = pos;
        it->second.Rotation = rot;
    }
}

void DefaultPhysicsWorld::GetTransform(PhysicsBodyHandle handle, glm::vec3& pos, glm::quat& rot)
{
    if (auto it = m_Bodies.find(handle); it != m_Bodies.end())
    {
        pos = it->second.Position;
        rot = it->second.Rotation;
    }
}

void DefaultPhysicsWorld::SetVelocity(PhysicsBodyHandle handle, const glm::vec3& velocity)
{
    if (auto it = m_Bodies.find(handle); it != m_Bodies.end())
    {
        it->second.Velocity = velocity;
    }
}

glm::vec3 DefaultPhysicsWorld::GetVelocity(PhysicsBodyHandle handle) const
{
    if (auto it = m_Bodies.find(handle); it != m_Bodies.end())
    {
        return it->second.Velocity;
    }
    return {0, 0, 0};
}

RaycastResult DefaultPhysicsWorld::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance)
{
    // Temporary stub until RaycastQuery is integrated here
    return RaycastResult();
}

void DefaultPhysicsWorld::Step(float fixedDt)
{
    // Simulation logic goes here (currently an empty stub 
    // waiting for code migration from CollisionCore and Dynamics)
    for (auto& [handle, body] : m_Bodies)
    {
        if (!body.IsKinematic)
        {
            if (body.UseGravity)
            {
                body.Velocity.y -= 9.8f * fixedDt;
            }
            body.Position += body.Velocity * fixedDt;
        }
    }
}

} // namespace Chained

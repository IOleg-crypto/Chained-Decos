#ifndef CH_IPHYSICS_WORLD_H
#define CH_IPHYSICS_WORLD_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "raycast_result.h"
#include "physics_types.h"

namespace Chained
{

struct PhysicsBodyDesc
{
    glm::vec3 Position = {0.0f, 0.0f, 0.0f};
    glm::quat Rotation = {1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 InitialVelocity = {0.0f, 0.0f, 0.0f};
    
    ColliderType Shape = ColliderType::Box;
    glm::vec3 Dimensions = {1.0f, 1.0f, 1.0f}; // For Sphere: x=radius. For Capsule: x=radius, y=height.
    
    float Mass = 1.0f;
    bool IsKinematic = false;
    bool UseGravity = true;
};

class IPhysicsWorld
{
public:
    virtual ~IPhysicsWorld() = default;

    virtual PhysicsBodyHandle CreateBody(const PhysicsBodyDesc& desc) = 0;
    virtual void DestroyBody(PhysicsBodyHandle handle) = 0;

    virtual void SetTransform(PhysicsBodyHandle handle, const glm::vec3& pos, const glm::quat& rot) = 0;
    virtual void GetTransform(PhysicsBodyHandle handle, glm::vec3& pos, glm::quat& rot) = 0;
    
    virtual void SetVelocity(PhysicsBodyHandle handle, const glm::vec3& velocity) = 0;
    virtual glm::vec3 GetVelocity(PhysicsBodyHandle handle) const = 0;

    virtual RaycastResult Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) = 0;

    virtual void Step(float fixedDt) = 0;
};

} // namespace Chained

#endif // CH_IPHYSICS_WORLD_H

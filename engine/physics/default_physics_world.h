#ifndef CH_DEFAULT_PHYSICS_WORLD_H
#define CH_DEFAULT_PHYSICS_WORLD_H

#include "iphysics_world.h"
#include "raycast_result.h"
#include <unordered_map>

namespace CHEngine
{

// Перевірка: чи потрібно нам дійсно виносити ВСЕ з entt зараз?
// Поки що DefaultPhysicsWorld буде адаптером, який може всередині мати спрощений реєстр 
// або просто список тіл.

struct DefaultPhysicsBody
{
    PhysicsBodyHandle Handle;
    glm::vec3 Position;
    glm::quat Rotation;
    glm::vec3 Velocity;
    float Mass;
    bool IsKinematic;
    bool UseGravity;
};

class DefaultPhysicsWorld : public IPhysicsWorld
{
public:
    DefaultPhysicsWorld();
    virtual ~DefaultPhysicsWorld() = default;

    PhysicsBodyHandle CreateBody(const PhysicsBodyDesc& desc) override;
    void DestroyBody(PhysicsBodyHandle handle) override;

    void SetTransform(PhysicsBodyHandle handle, const glm::vec3& pos, const glm::quat& rot) override;
    void GetTransform(PhysicsBodyHandle handle, glm::vec3& pos, glm::quat& rot) override;
    
    void SetVelocity(PhysicsBodyHandle handle, const glm::vec3& velocity) override;
    glm::vec3 GetVelocity(PhysicsBodyHandle handle) const override;

    RaycastResult Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) override;

    void Step(float fixedDt) override;

private:
    std::unordered_map<PhysicsBodyHandle, DefaultPhysicsBody> m_Bodies;
    PhysicsBodyHandle m_NextHandle = 1;
};

} // namespace CHEngine

#endif // CH_DEFAULT_PHYSICS_WORLD_H

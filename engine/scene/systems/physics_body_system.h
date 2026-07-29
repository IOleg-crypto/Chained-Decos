#ifndef CH_PHYSICS_BODY_SYSTEM_H
#define CH_PHYSICS_BODY_SYSTEM_H

#include <entt/entt.hpp>

namespace Chained
{
class IPhysicsWorld;

namespace PhysicsBodySystem
{
void RegisterObservers(entt::registry& reg);
void OnRigidBodyConstruct(entt::registry& reg, entt::entity e);
void BatchInitializeBodies(entt::registry& reg, IPhysicsWorld* world);
void Update(entt::registry& reg);
} // namespace PhysicsBodySystem
} // namespace Chained

#endif // CH_PHYSICS_BODY_SYSTEM_H

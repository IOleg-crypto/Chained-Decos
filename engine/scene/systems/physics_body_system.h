#ifndef CH_PHYSICS_BODY_SYSTEM_H
#define CH_PHYSICS_BODY_SYSTEM_H

#include "engine/physics/iphysics_world.h"
#include "engine/scene/components/physics/physics_component.h"
#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace Chained
{
	class IPhysicsWorld;

	namespace PhysicsBodySystem
	{
		void ApplyAutoCalculate(entt::entity entity, entt::registry& registry, ColliderComponent& collider,
								const glm::vec3& scale);
		bool BuildBodyDesc(entt::registry& reg, entt::entity e, PhysicsBodyDesc& outDesc);
		void BatchInitializeBodies(entt::registry& reg, IPhysicsWorld* world);
		void Update(entt::registry& reg);
	} // namespace PhysicsBodySystem
} // namespace Chained

#endif // CH_PHYSICS_BODY_SYSTEM_H

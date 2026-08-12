// Physics component registrations (RigidBody, Collider, Primitive)
// Split into its own TU to reduce obj file size in MinGW Clang Debug builds.
#include "component_registry.h"
#include "components/physics/physics_component.h"
#include "components/render/primitive_component.h"
#include "thirdparty/IconsFontAwesome6.h"

namespace Chained
{
	void RegisterPhysicsComponents()
	{
		ComponentRegistry::RegisterReflective<RigidBodyComponent>("Rigid Body", ICON_FA_CUBES, "Physics");
		ComponentRegistry::RegisterReflective<ColliderComponent>("Collider", ICON_FA_SHIELD, "Physics");
		ComponentRegistry::RegisterReflective<PrimitiveComponent>("Primitive", nullptr, "Physics");
	}
} // namespace Chained

#ifndef CH_PHYSICS_TYPES_H
#define CH_PHYSICS_TYPES_H

#include <cstdint>

namespace Chained
{
	/// Opaque handle identifying a Jolt physics body. Wraps the Jolt BodyID index.
	using PhysicsBodyHandle = uint64_t;

	/// Sentinel value indicating an invalid/uninitialized body handle.
	constexpr PhysicsBodyHandle kInvalidPhysicsBody = 0;

	/// Supported collider shape types.
	enum class ColliderType
	{
		Box,	 ///< Axis-aligned box defined by half-extents.
		Sphere,	 ///< Sphere defined by a single radius.
		Capsule, ///< Vertical capsule defined by radius + half-height.
		Mesh	 ///< Triangle mesh (typically for static level geometry).
	};
} // namespace Chained

#endif // CH_PHYSICS_TYPES_H

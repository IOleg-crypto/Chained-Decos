#ifndef CH_IPHYSICS_WORLD_H
#define CH_IPHYSICS_WORLD_H

#include "physics_types.h"
#include "raycast_result.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace Chained
{

	// A single triangle for building a MeshShape collider.
	struct PhysicsTriangle
	{
		glm::vec3 V0, V1, V2;
	};

	// Description used to create a single physics body.
	struct PhysicsBodyDesc
	{
		glm::vec3 Position = {0.0f, 0.0f, 0.0f};
		glm::quat Rotation = {1.0f, 0.0f, 0.0f, 0.0f};
		glm::vec3 InitialVelocity = {0.0f, 0.0f, 0.0f};
		glm::vec3 Offset = {0.0f, 0.0f, 0.0f};
		uint64_t UserData = 0;

		// Shape
		ColliderType Shape = ColliderType::Box;
		glm::vec3 Dimensions = {1.0f, 1.0f,
								1.0f}; // Box: half-extents. Sphere: x=radius. Capsule: x=radius, y=half-height.
		std::vector<PhysicsTriangle> Triangles; // Only used when Shape == ColliderType::Mesh.
		glm::vec3 MeshScale = {
			1.0f, 1.0f, 1.0f}; // Mesh only: scale applied via ScaledShape so the cached BVH stays scale-independent.
							   // Triangles must therefore be supplied in unscaled model-local space.
		std::string
			CacheKey; // Optional: enables mesh shape caching. With MeshScale decoupled this can be just the model path.

		// Body properties
		float Mass = 1.0f;
		float LinearDamping = 0.01f;
		float AngularDamping = 0.05f;
		float Friction = 0.5f;
		float Restitution = 0.0f;

		bool IsKinematic = false;
		bool IsStatic = false;
		bool UseGravity = true;
		bool IsFixedRotation = false;
		bool UseFastBuildQuality = true;
	};

	class IPhysicsWorld
	{
	public:
		virtual ~IPhysicsWorld() = default;

		virtual PhysicsBodyHandle CreateBody(const PhysicsBodyDesc& desc) = 0;

		/// Creates multiple bodies in one batch.
		/// Each backend implements its own optimized path (e.g. Jolt's CreateBodies/AddBodies).
		virtual std::vector<PhysicsBodyHandle> CreateBodies(const std::vector<PhysicsBodyDesc>& descs) = 0;

		virtual void DestroyBody(PhysicsBodyHandle handle) = 0;

		/// Returns true if a mesh shape with the given cache key already exists.
		/// Use to skip triangle extraction on cache hits.
		virtual bool HasCachedMeshShape(const std::string& key) const = 0;

		/// Returns true if a shape is currently being built in the background.
		virtual bool IsShapeBaking(const std::string& key) const = 0;

		/// Returns true if any background shape bakes are in flight.
		virtual bool HasPendingShapeBakes() const = 0;

		/// Pre-builds and caches a collision shape (BVH or Convex Hull) thread-safely on a worker thread.
		virtual void PrebuildShape(const PhysicsBodyDesc& desc) = 0;

		/// Queues non-blocking background baking of a shape via ThreadPool if not already cached or baking.
		virtual void QueuePrebuildShape(const PhysicsBodyDesc& desc) = 0;

		virtual void SetTransform(PhysicsBodyHandle handle, const glm::vec3& pos, const glm::quat& rot) = 0;
		virtual void GetTransform(PhysicsBodyHandle handle, glm::vec3& pos, glm::quat& rot) = 0;

		virtual void SetVelocity(PhysicsBodyHandle handle, const glm::vec3& velocity) = 0;
		virtual glm::vec3 GetVelocity(PhysicsBodyHandle handle) const = 0;

		virtual RaycastResult Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) = 0;

		virtual void Step(float fixedDt) = 0;

		virtual void SetGravity(float gravity) = 0;

		/// Returns true if the body identified by @p handle is currently resting on a surface
		/// whose contact normal points upward (i.e. the body is "grounded").
		virtual bool IsBodyGrounded(PhysicsBodyHandle handle) const = 0;

		/// Returns true if the body is awake (actively simulated). Sleeping bodies retain
		/// their last grounded state and do not produce fresh contact callbacks.
		virtual bool IsBodyActive(PhysicsBodyHandle handle) const = 0;

		/// Clears the grounded-state tracker so it can be rebuilt from contacts during the next Step().
		virtual void ClearGroundedState() = 0;
	};

} // namespace Chained

#endif // CH_IPHYSICS_WORLD_H

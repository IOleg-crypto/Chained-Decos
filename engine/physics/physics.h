#ifndef CH_PHYSICS_H
#define CH_PHYSICS_H

#include "engine/core/service.h"
#include "engine/common/base.h"
#include "engine/common/timestep.h"
#include "engine/physics/raycast_result.h"
#include "engine/scene/components.h"
#include "iphysics_world.h"
#include <entt/entt.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Chained
{
	class Scene;
	class IPhysicsWorld;

	// High-level physics module that owns the Jolt world and orchestrates
	// body creation, fixed-timestep stepping, and component synchronization.
	class CH_API Physics : public Service
	{
	public:
		Physics();
		virtual ~Physics() override;

		// Service lifecycle
		virtual void Initialize() override;
		virtual void Shutdown() override;

		// Core API

		/// Returns the Jolt world, creating it lazily on first access.
		IPhysicsWorld* GetWorld();

		/// Destroys the current world and creates a fresh one, applying
		/// gravity from the active project configuration.
		/// If scene is provided, invalidates all RigidBodyComponent handles.
		void ResetWorld(Scene* scene = nullptr);

		/// Iterates all entities with TransformComponent + RigidBodyComponent
		/// that don't yet have a physics body, and creates Jolt bodies for them.
		void InitializeBodies(Scene* scene);

		/// Runs the fixed-timestep physics loop (up to kMaxStepsPerFrame sub-steps),
		/// then synchronizes dynamic body transforms and velocities back to components.
		void Update(Scene* scene, Timestep deltaTime, bool runtime = false);

		/// Casts a ray through the Jolt world and returns the closest hit.
		RaycastResult Raycast(Ray ray);

		/// Sets the velocity on a body unconditionally. Use for respawn teleports.
		void ForceSetVelocity(PhysicsBodyHandle handle, const glm::vec3& velocity);

		// Scene context helpers
		void ResetAccumulator(Scene* scene);
		void ClearContext(Scene* scene);

	private:
		/// Reads back position, rotation, velocity, and grounded state from Jolt
		/// for all Dynamic bodies. Static and Kinematic bodies are skipped because
		/// their transforms are driven by scripts or remain fixed.
		void UpdateColliders(Scene* scene);

	private:
		std::unique_ptr<IPhysicsWorld> m_World;

		// Per-scene fixed-timestep accumulator (replaces PhysicsContext in entt ctx)
		std::unordered_map<Scene*, float> m_Accumulators;
	};

} // namespace Chained

#endif // CH_PHYSICS_H

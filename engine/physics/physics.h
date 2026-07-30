#ifndef CH_PHYSICS_H
#define CH_PHYSICS_H

#include "engine/core/engine_module.h"
#include "engine/common/base.h"
#include "engine/common/timestep.h"
#include "engine/physics/raycast_result.h"
#include "engine/scene/components.h"
#include "iphysics_world.h"
#include <entt/entt.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Chained
{
class Scene;
class IPhysicsWorld;
struct PhysicsTriangle;

// Per-scene physics context stored in the EnTT registry ctx.
// Holds the fixed-timestep accumulator and an optional collision callback.
struct PhysicsContext
{
    float Accumulator = 0.0f;
    std::function<void(entt::entity, entt::entity)> CollisionCallback;
};

// High-level physics module that owns the Jolt world and orchestrates
// body creation, fixed-timestep stepping, and component synchronization.
class CH_API Physics : public EngineModule
{
public:
    Physics();
    virtual ~Physics() override;

    // EngineModule lifecycle
    virtual void Initialize() override;
    virtual void Update(Timestep ts)
    {
    }
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
    RaycastResult Raycast(Scene* scene, Ray ray);

    /// When AutoCalculate=true: reads model bounding box and writes Size/Offset/Radius/Height
    /// back into the collider. Call during body initialization.
    void ApplyAutoCalculate(entt::entity entity, entt::registry& registry, ColliderComponent& collider,
                            const glm::vec3& scale);

    /// Unconditionally sets the velocity on a body, bypassing the Y-override
    /// that the normal script-path applies.  Use for respawn teleports.
    void ForceSetVelocity(PhysicsBodyHandle handle, const glm::vec3& velocity);

    // Scene context helpers
    PhysicsContext& GetContext(Scene* scene);
    void ResetAccumulator(Scene* scene);
    void ClearContext(Scene* scene);
    void SetCollisionCallback(Scene* scene, std::function<void(entt::entity, entt::entity)> callback);

private:
    /// Reads back position, rotation, velocity, and grounded state from Jolt
    /// for all Dynamic bodies. Static and Kinematic bodies are skipped because
    /// their transforms are driven by scripts or remain fixed.
    void UpdateColliders(Scene* scene);

private:
    std::unique_ptr<IPhysicsWorld> m_World;
};

} // namespace Chained

#endif // CH_PHYSICS_H

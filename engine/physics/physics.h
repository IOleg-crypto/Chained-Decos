#ifndef CH_PHYSICS_H
#define CH_PHYSICS_H

#include "engine/core/base.h"
#include "engine/core/timestep.h"
#include "entt/entt.hpp"
#include <functional>
#include <memory>
#include <string>
#include "raycast_result.h"

namespace CHEngine
{
class Scene;
class BVH;

struct PhysicsContext
{
    /** Accumulates fixed-step simulation time for this scene. */
    float Accumulator = 0.0f;
    /** Optional callback invoked when two entities begin colliding. */
    std::function<void(entt::entity, entt::entity)> CollisionCallback;
};

class Physics
{
public: // Lifecycle
    /** Initializes the global physics subsystem. */
    static void Init();
    /** Shuts the physics subsystem down and clears global state. */
    static void Shutdown();
    /** Returns true once the physics subsystem has been initialized. */
    static bool IsInitialized();

public: // BVH cache API
    /** Returns the cached BVH for the given asset path, if one exists. */
    static std::shared_ptr<BVH> GetBVH(const std::string& path);
    /** Removes the cached BVH entry for the given asset path. */
    static void InvalidateBVH(const std::string& path);
    /** Replaces or inserts the cached BVH for the given asset path. */
    static void UpdateBVHCache(const std::string& path, std::shared_ptr<BVH> bvh);

public: // Simulation & Queries
    /** Steps the physics simulation and updates collider states for a given scene. */
    static void Update(Scene* scene, Timestep deltaTime, bool runtime = false);

    /** Performs a spatial raycast query within the given scene. */
    static RaycastResult Raycast(Scene* scene, Ray ray);

    /** Returns the mutable physics context associated with the scene. */
    static PhysicsContext& GetContext(Scene* scene);
    /** Resets the fixed-step accumulator for the scene. */
    static void ResetAccumulator(Scene* scene);
    /** Clears all cached physics context for the scene. */
    static void ClearContext(Scene* scene);
    /** Sets the collision callback invoked by the physics step. */
    static void SetCollisionCallback(Scene* scene, std::function<void(entt::entity, entt::entity)> callback);

private: // Internal Helpers
    static void UpdateColliders(Scene* scene);
    static void ResolveSimulation(Scene* scene, Timestep deltaTime);
};
} // namespace CHEngine

#endif // CH_PHYSICS_H

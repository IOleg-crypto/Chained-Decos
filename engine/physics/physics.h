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
    float Accumulator = 0.0f;
    std::function<void(entt::entity, entt::entity)> CollisionCallback;
};

class Physics
{
public: // Lifecycle
    static void Init();
    static void Shutdown();
    static bool IsInitialized();

public: // BVH cache API
    static std::shared_ptr<BVH> GetBVH(const std::string& path);
    static void InvalidateBVH(const std::string& path);
    static void UpdateBVHCache(const std::string& path, std::shared_ptr<BVH> bvh);

public: // Simulation & Queries
    // Steps the physics simulation and updates collider states for a given scene.
    static void Update(Scene* scene, Timestep deltaTime, bool runtime = false);

    // Performs a spatial raycast query within the given scene.
    static RaycastResult Raycast(Scene* scene, Ray ray);

    // Context management helpers
    static PhysicsContext& GetContext(Scene* scene);
    static void ResetAccumulator(Scene* scene);
    static void ClearContext(Scene* scene);
    static void SetCollisionCallback(Scene* scene, std::function<void(entt::entity, entt::entity)> callback);

private: // Internal Helpers
    static void UpdateColliders(Scene* scene);
    static void ResolveSimulation(Scene* scene, Timestep deltaTime);
};
} // namespace CHEngine

#endif // CH_PHYSICS_H

#ifndef CH_PHYSICS_H
#define CH_PHYSICS_H

#include "engine/core/base.h"
#include "engine/core/timestep.h"
#include "entt/entt.hpp"
#include "raylib.h"
#include <future>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace CHEngine
{
class Scene;
class ModelAsset;
class BVH;

// Represents the physics simulation and spatial query context for a specific scene.
// Following the 'Action-Based' naming convention (Physics instead of PhysicsSystem).
// Organized with encapsulated instance state.
struct RaycastResult
{
    bool Hit = false;
    float Distance = 0.0f;
    Vector3 Position = {0.0f, 0.0f, 0.0f};
    Vector3 Normal = {0.0f, 0.0f, 0.0f};
    entt::entity Entity = entt::null;
    int MeshIndex = -1;
};

class Physics
{
public: // Life Cycle
    Physics(Scene* scene);
    ~Physics();

    // Initializes the physics context for the scene.
    static void Init(); // Global engine-level init if needed

    // Shuts down global physics resources.
    static void Shutdown();

public: // Simulation & Queries
    // Steps the physics simulation and updates collider states.
    void Update(Timestep deltaTime, bool runtime = false);

    // Performs a spatial raycast query within the owned scene.
    RaycastResult Raycast(Ray ray);

    // Retrieves or starts building a BVH for a model asset.
    std::shared_ptr<BVH> GetBVH(ModelAsset* asset);

    // Invalidates the cached BVH for a specific asset, forcing a rebuild on next update.
    void InvalidateBVH(ModelAsset* asset);

    // Directly updates the cache with a pre-built BVH.
    void UpdateBVHCache(ModelAsset* asset, std::shared_ptr<BVH> bvh);

private: // Internal Helpers
    void UpdateColliders();
    void ResolveSimulation(Timestep deltaTime);

private: // Members
    Scene* m_Scene = nullptr;

    std::unique_ptr<class NarrowPhase> m_NarrowPhase;
    std::unique_ptr<class Dynamics> m_Dynamics;
    std::unique_ptr<class SceneTrace> m_SceneTrace;

    // Localized BVH cache to avoid global static state
    std::unordered_map<ModelAsset*, std::shared_future<std::shared_ptr<BVH>>> m_BVHCache;
    mutable std::mutex m_BVHMutex;
    float m_Accumulator = 0.0f;
};
} // namespace CHEngine

#endif // CH_PHYSICS_H

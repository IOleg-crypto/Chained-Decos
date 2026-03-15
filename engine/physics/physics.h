#ifndef CH_PHYSICS_H
#define CH_PHYSICS_H

#include "engine/core/base.h"
#include "engine/core/timestep.h"
#include "entt/entt.hpp"
#include "raylib.h"
#include <functional>
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

class PhysicsSystem
{
public:
    PhysicsSystem();
    ~PhysicsSystem();

    void Init();
    void Shutdown();

    static PhysicsSystem& Get();

    // BVH Cache moved to global system
    std::shared_ptr<BVH> GetBVH(ModelAsset* asset);
    void InvalidateBVH(ModelAsset* asset);
    void UpdateBVHCache(ModelAsset* asset, std::shared_ptr<BVH> bvh);

private:
    // Localized BVH cache in global system
    std::unordered_map<ModelAsset*, std::shared_future<std::shared_ptr<BVH>>> m_BVHCache;
    mutable std::mutex m_BVHMutex;

    // Persistent asset cache for collider shape computation (avoids per-frame allocation)
    // This can still be global or part of AssetManager, but keeping here for now as shared state
    std::unordered_map<std::string, std::shared_ptr<ModelAsset>> m_ColliderAssetCache;
};

struct PhysicsContext
{
    float Accumulator = 0.0f;
    std::function<void(entt::entity, entt::entity)> CollisionCallback;
};

class Physics
{
public: // Simulation & Queries
    // Steps the physics simulation and updates collider states for a given scene.
    static void Update(Scene* scene, Timestep deltaTime, bool runtime = false);

    // Performs a spatial raycast query within the given scene.
    static RaycastResult Raycast(Scene* scene, Ray ray);

    // Context management helpers
    static PhysicsContext& GetContext(Scene* scene);
    static void SetCollisionCallback(Scene* scene, std::function<void(entt::entity, entt::entity)> callback);

private: // Internal Helpers
    static void UpdateColliders(Scene* scene);
    static void ResolveSimulation(Scene* scene, Timestep deltaTime);
};
} // namespace CHEngine

#endif // CH_PHYSICS_H

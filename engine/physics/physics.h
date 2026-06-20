#ifndef CH_PHYSICS_H
#define CH_PHYSICS_H

#include "engine/foundation/base.h"
#include "engine/foundation/timestep.h"
#include "engine/physics/collision_core.h"
#include "engine/physics/raycast_result.h"
#include "engine/physics/physics_config.h"
#include <entt/entt.hpp>
#include <functional>
#include <memory>
#include <string>
#include "raycast_result.h"
#include "engine/core/engine_module.h"

// TODO : future refactor (when link Jolt physics)
namespace Chained
{
class Scene;
class BVH;
class ModelAsset;

// Per-scene physics state shared across simulation frames.
struct PhysicsContext
{
    // Accumulates fixed-step simulation time for this scene.
    float Accumulator = 0.0f;
    // Optional callback invoked when two entities begin colliding.
    std::function<void(entt::entity, entt::entity)> CollisionCallback;
};

class CH_API Physics : public EngineModule
{
public: // Lifecycle
    // Initializes the global physics subsystem.
    virtual void Initialize() override;
    virtual void Update(Timestep ts) override {}
public:
    Physics();
    virtual ~Physics() override;

    // Shuts the physics subsystem down and clears global state.
    virtual void Shutdown() override;
    // Returns the singleton instance.
    static Physics& Get();
    // Returns true once the physics subsystem has been initialized.
    static bool IsInitialized();

    // Returns the active physics world implementation.
    IPhysicsWorld* GetWorld();

    // Initializes physics bodies for entities that haven't been created yet.
    void InitializeBodies(Scene* scene);

public: // BVH cache API
    // Returns the cached BVH for the given asset path, if one exists.
    static std::shared_ptr<BVH> GetBVH(const std::shared_ptr<ModelAsset>& asset);
    static std::shared_ptr<BVH> GetBVH(const std::string& modelPath);
    // Removes the cached BVH entry for the given asset path.
    static void InvalidateBVH(const std::string& path);
    // Replaces or inserts the cached BVH for the given asset path.
    static void UpdateBVHCache(const std::string& path, std::shared_ptr<BVH> bvh);

public: // Simulation & Queries
    // Steps the physics simulation and updates collider states for a given scene.
    static void Update(Scene* scene, Timestep deltaTime, bool runtime = false);

    // Performs a spatial raycast query within the given scene.
    static RaycastResult Raycast(Scene* scene, Ray ray);

    // Returns the mutable physics context associated with the scene.
    static PhysicsContext& GetContext(Scene* scene);
    // Resets the fixed-step accumulator for the scene.
    static void ResetAccumulator(Scene* scene);
    // Clears all cached physics context for the scene.
    static void ClearContext(Scene* scene);
    // Sets the collision callback invoked by the physics step.
    static void SetCollisionCallback(Scene* scene, std::function<void(entt::entity, entt::entity)> callback);

private: // Internal Helpers
    static void UpdateColliders(Scene* scene);
    static void ResolveSimulation(Scene* scene, Timestep deltaTime, float gravity);

private:
    std::unique_ptr<IPhysicsWorld> m_World;
    static Physics* s_Instance;
};
} // namespace Chained

#endif // CH_PHYSICS_H

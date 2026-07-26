#ifndef CH_SCENE_RESOURCE_MANAGER_H
#define CH_SCENE_RESOURCE_MANAGER_H


#include "engine/common/timestep.h"
#include "engine/physics/iphysics_world.h"
#include <entt/entt.hpp>
#include <vector>

namespace Chained
{
class Scene;

namespace SceneResources
{
    /**
     * @brief Registers observers for sprite, shader, and model components to resolve assets.
     */
    void RegisterObservers(entt::registry& reg);

    /**
     * @brief Performs frame updates for asset resolution, animation playback, and audio syncing.
     */
    void Update(entt::registry& reg, Timestep ts);

    /**
     * @brief Lifecycle hooks for runtime simulation.
     */
    void OnRuntimeStart(Scene* scene);
    void OnRuntimeStop(Scene* scene);

    /**
     * @brief Callback invoked when a RigidBodyComponent is constructed.
     * Used to initialize physics bodies immediately during runtime.
     */
    void OnRigidBodyConstruct(entt::registry& reg, entt::entity e);

    /**
     * @brief Builds a PhysicsBodyDesc for an entity with RigidBody + Collider components.
     * Returns true on success, false if the entity is missing components or assets aren't ready.
     * Does NOT create the physics body — caller is responsible for that.
     */
    bool BuildBodyDesc(entt::registry& reg, entt::entity e, PhysicsBodyDesc& outDesc);

    /**
     * @brief Batch-creates physics bodies for all pending entities, sorted static-first.
     * Uses the batch API for efficient broadphase insertion.
     */
    void BatchInitializeBodies(entt::registry& reg, IPhysicsWorld* world);

    // Asset resolution methods
    void ResolveSprite(entt::registry& reg, entt::entity e);
    void ResolveShader(entt::registry& reg, entt::entity e);
    void ResolveModel(entt::registry& reg, entt::entity e);

    /**
     * @brief (Re)builds the ModelAsset backing a PrimitiveComponent from its parameters.
     * Runs on the main thread (creates a VAO via ModelAsset::OnLoaded). Clears the Dirty flag.
     */
    void ResolvePrimitive(entt::registry& reg, entt::entity e);

    /**
     * @brief Re-applies textures from AssetManager to an existing primitive asset without
     * rebuilding mesh geometry. Called each frame while TexturesPending is true.
     */
    void ApplyPrimitiveTextures(entt::registry& reg, entt::entity e);

    /**
     * @brief Marks a PrimitiveComponent dirty so Update() regenerates its mesh next frame.
     * Connected to on_construct/on_update; does not touch the GPU (safe off the main thread).
     */
    void MarkPrimitiveDirty(entt::registry& reg, entt::entity e);
}

} // namespace Chained

#endif // CH_SCENE_RESOURCE_MANAGER_H

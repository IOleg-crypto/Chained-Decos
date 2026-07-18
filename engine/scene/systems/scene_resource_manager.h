#ifndef CH_SCENE_RESOURCE_MANAGER_H
#define CH_SCENE_RESOURCE_MANAGER_H


#include "engine/common/timestep.h"
#include <entt/entt.hpp>

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
     * @brief Marks a PrimitiveComponent dirty so Update() regenerates its mesh next frame.
     * Connected to on_construct/on_update; does not touch the GPU (safe off the main thread).
     */
    void MarkPrimitiveDirty(entt::registry& reg, entt::entity e);
}

} // namespace Chained

#endif // CH_SCENE_RESOURCE_MANAGER_H

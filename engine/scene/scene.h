#ifndef CH_SCENE_H
#define CH_SCENE_H

#include "components.h"
#include "engine/graphics/api/camera_types.h"
#include "engine/core/ch_assert.h"
#include "engine/core/base.h"
#include "engine/core/events.h"
#include "engine/core/timestep.h"
#include "engine/scene/entity.h"
#include "engine/scene/scene_settings.h"
#include "entt/entt.hpp"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace CHEngine
{
class Physics;

class Scene : public std::enable_shared_from_this<Scene>
{
public:
    Scene();
    ~Scene();

    /**
     * Creates a deep copy of the scene state, including entities and scene settings.
     * Runtime-only state is rebuilt by the copy path.
     */
    static std::shared_ptr<Scene> Copy(std::shared_ptr<Scene> other);

    /** Creates a regular world entity with an optional display name. */
    Entity CreateEntity(const std::string& name = std::string()) { return m_Manager.Create(name); }
    /** Creates an entity with a stable UUID, used for serialization and duplication. */
    Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string()) { return m_Manager.CreateWithUUID(uuid, name); }
    /** Creates a UI entity that uses the UI-specific hierarchy and component setup. */
    Entity CreateUIEntity(const std::string& type, const std::string& name = std::string()) { return m_Manager.CreateUI(type, name); }
    /** Copies an entity from this scene's registry into a new entity. */
    Entity CopyEntity(entt::entity copyEntity) { return m_Manager.Copy(copyEntity); }
    /** Destroys an entity and all of its components. */
    void DestroyEntity(Entity entity) { entity.Destroy(); }

    /** Finds the first entity whose TagComponent matches the requested tag. */
    Entity FindEntityByTag(const std::string& tag) { return m_Manager.FindByTag(tag); }
    /** Returns the entity associated with the given UUID, or an invalid entity if missing. */
    Entity GetEntityByUUID(UUID uuid) { return m_Manager.GetByUUID(uuid); }

public: // Life Cycle & Simulation
    /** Starts runtime execution for the scene. */
    void OnRuntimeStart();
    /** Stops runtime execution and releases runtime-only scene state. */
    void OnRuntimeStop();
    /** Advances runtime simulation, scripts, physics, animation, and audio. */
    void OnUpdateRuntime(Timestep timestep);
    /** Advances editor-time preview systems without entering runtime mode. */
    void OnUpdateEditor(Timestep timestep);
    /** Updates viewport-dependent scene state such as cameras and render targets. */
    void OnViewportResize(uint32_t width, uint32_t height);

    bool IsSimulationRunning() const
    {
        return m_IsSimulationRunning;
    }
    /** Dispatches scene-level events to the active systems and components. */
    void OnEvent(Event& event);

public: // Scene Settings
    /** Returns mutable scene settings for editor/runtime code. */
    SceneSettings& GetSettings()
    {
        return m_Settings;
    }
    /** Returns read-only scene settings. */
    const SceneSettings& GetSettings() const
    {
        return m_Settings;
    }


public: // Systems & Tools
    /** Returns the live registry backing this scene. */
    entt::registry& GetRegistry()
    {
        return m_Manager.GetRegistry();
    }
    /** Returns the live registry backing this scene. */
    const entt::registry& GetRegistry() const
    {
        return m_Manager.GetRegistry();
    }
    
    /** Returns a shared registry handle for code that needs to keep the registry alive. */
    std::shared_ptr<entt::registry> GetRegistryPtr()
    {
        return m_Manager.GetRegistryPtr();
    }

    /** Returns the active camera for this scene, if one is available. */
    std::optional<Camera3D> GetActiveCamera();
    /** Returns the primary camera entity, or an invalid entity if none is marked primary. */
    Entity GetPrimaryCameraEntity();

private:
    Camera3D GetCameraFromEntity(entt::entity entityHandle);

private:
    Entity m_Manager;
    SceneSettings m_Settings;

    bool m_IsSimulationRunning = false;


    void OnIDConstruct(entt::registry& registry, entt::entity entity);
    void OnIDDestroy(entt::registry& registry, entt::entity entity);

    // Hierarchy Handlers
    void OnHierarchyDestroy(entt::registry& registry, entt::entity entity);
    
    // Script Cleanup Handlers

private: // Update Logic
    void UpdatePhysics(Timestep deltaTime);
    void UpdateAnimations(Timestep deltaTime);
    void UpdateAudio(Timestep deltaTime);
    void UpdateHierarchy();

    friend class Entity;
};

} // namespace CHEngine

#endif // CH_SCENE_H

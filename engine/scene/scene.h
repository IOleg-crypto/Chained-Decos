#ifndef CH_SCENE_H
#define CH_SCENE_H

#include "components.h"
#include "engine/core/assert.h"
#include "engine/core/base.h"
#include "engine/core/events.h"
#include "engine/core/timestep.h"
#include "engine/scene/entity.h"
#include "engine/scene/scene_settings.h"
#include "entt/entt.hpp"
#include <memory>
#include <optional>
#include <raylib.h>
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

    static std::shared_ptr<Scene> Copy(std::shared_ptr<Scene> other);

    Entity CreateEntity(const std::string& name = std::string()) { return m_Manager.Create(name); }
    Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string()) { return m_Manager.CreateWithUUID(uuid, name); }
    Entity CreateUIEntity(const std::string& type, const std::string& name = std::string()) { return m_Manager.CreateUI(type, name); }
    Entity CopyEntity(entt::entity copyEntity) { return m_Manager.Copy(copyEntity); }
    void DestroyEntity(Entity entity) { entity.Destroy(); }

    Entity FindEntityByTag(const std::string& tag) { return m_Manager.FindByTag(tag); }
    Entity GetEntityByUUID(UUID uuid) { return m_Manager.GetByUUID(uuid); }

public: // Life Cycle & Simulation
    void OnRuntimeStart();
    void OnRuntimeStop();
    void OnUpdateRuntime(Timestep timestep);
    void OnUpdateEditor(Timestep timestep);
    void OnViewportResize(uint32_t width, uint32_t height);

    bool IsSimulationRunning() const
    {
        return m_IsSimulationRunning;
    }
    void OnEvent(Event& event);

public: // Scene Settings
    SceneSettings& GetSettings()
    {
        return m_Settings;
    }
    const SceneSettings& GetSettings() const
    {
        return m_Settings;
    }


public: // Systems & Tools
    entt::registry& GetRegistry()
    {
        return m_Manager.GetRegistry();
    }
    const entt::registry& GetRegistry() const
    {
        return m_Manager.GetRegistry();
    }
    
    std::shared_ptr<entt::registry> GetRegistryPtr()
    {
        return m_Manager.GetRegistryPtr();
    }

    std::optional<Camera3D> GetActiveCamera();
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
    void UpdateHierarchy();

    friend class Entity;
};

} // namespace CHEngine

#endif // CH_SCENE_H

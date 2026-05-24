#ifndef CH_SCENE_H
#define CH_SCENE_H

#include "components.h"
#include "engine/core/base.h"
#include "engine/core/ch_assert.h"
#include "engine/core/events.h"
#include "engine/core/timestep.h"
#include "engine/graphics/api/camera_types.h"
#include "engine/scene/entity.h"
#include "engine/scene/scene_settings.h"
#include "entt/entt.hpp"
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include <filesystem>

 

namespace CHEngine
{
class ScriptEngine;
class SceneScriptingManager;
class SceneSystemManager;

// Owns the scene registry, scene settings, and the runtime/editor update bridge.
class CH_API Scene
{
public:
    Scene(ScriptEngine* scriptEngine = nullptr);
    ~Scene();

public:
    // Creates a new scene with default entities (e.g. Main Camera).
    static std::shared_ptr<Scene> CreateDefault();
    // Creates a deep copy of another scene.
    static std::shared_ptr<Scene> Copy(std::shared_ptr<Scene> other);

    virtual void OnEvent(Event& e);
    virtual void OnRenderUI();

public:
    Entity CreateEntity(const std::string& name = std::string());
    Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
    Entity CreateUIEntity(const std::string& type, const std::string& name = std::string());

public:
    Entity CopyEntity(entt::entity copyEntity);
    void DestroyEntity(Entity entity);
    Entity FindEntityByTag(const std::string& tag);
    Entity GetEntityByUUID(UUID uuid);

public: // Life Cycle & Simulation
    void OnRuntimeStart();
    void OnRuntimeStop();
    void OnUpdateRuntime(Timestep timestep);
    void OnUpdateEditor(Timestep timestep);
    void OnViewportResize(uint32_t width, uint32_t height);

public:
    bool IsSimulationRunning() const;
    ScriptEngine* GetScriptEngine() const;

public:
    SceneSettings& GetSettings();
    const SceneSettings& GetSettings() const;

    std::vector<entt::entity>& GetRootEntities() { return m_RootEntities; }
    const std::vector<entt::entity>& GetRootEntities() const { return m_RootEntities; }

public: // Systems & Tools
    entt::registry& GetRegistry();
    const entt::registry& GetRegistry() const;
    entt::registry* GetRegistryPtr();

    

private:
    std::unique_ptr<entt::registry> m_Registry;
    SceneSettings m_Settings;
    bool m_IsSimulationRunning = false;
    std::unique_ptr<SceneScriptingManager> m_ScriptingManager;
    std::unique_ptr<SceneSystemManager> m_SystemManager;

    void OnIDConstruct(entt::registry& registry, entt::entity entity);
    void OnIDDestroy(entt::registry& registry, entt::entity entity);
    // Hierarchy handlers.
    void OnHierarchyDestroy(entt::registry& registry, entt::entity entity);

    Entity CopyEntityInternal(entt::entity copyEntity, entt::entity parentEntity = entt::null);

    friend class Entity;
    friend class HierarchySystem;
    friend class SceneSerializer;

    std::vector<entt::entity> m_RootEntities;
    
    
};

} // namespace CHEngine

#endif // CH_SCENE_H


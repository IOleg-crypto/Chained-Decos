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
#include <string>
#include <unordered_map>
#include <vector>

namespace CHEngine
{
class SceneScriptingManager;
// Owns the scene registry, scene settings, and the runtime/editor update bridge.
class Scene
{
public:
    Scene();
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

public:
    SceneSettings& GetSettings();
    const SceneSettings& GetSettings() const;

public: // Systems & Tools
    entt::registry& GetRegistry();
    const entt::registry& GetRegistry() const;
    std::shared_ptr<entt::registry> GetRegistryPtr();

public:
    std::optional<Camera3D> GetActiveCamera();
    Entity GetPrimaryCameraEntity();

private:
    Camera3D GetCameraFromEntity(entt::entity entityHandle);

    SceneScriptingManager& GetScriptingManager() { return *m_ScriptingManager; }

private:
    std::shared_ptr<entt::registry> m_Registry;
    SceneSettings m_Settings;
    bool m_IsSimulationRunning = false;
    std::unique_ptr<SceneScriptingManager> m_ScriptingManager;

private:
    void OnIDConstruct(entt::registry& registry, entt::entity entity);
    void OnIDDestroy(entt::registry& registry, entt::entity entity);
    // Hierarchy handlers.
    void OnHierarchyDestroy(entt::registry& registry, entt::entity entity);
    
    Entity CopyEntityInternal(entt::entity copyEntity, entt::entity parentEntity = entt::null);

    friend class Entity;
};

} // namespace CHEngine

#endif // CH_SCENE_H

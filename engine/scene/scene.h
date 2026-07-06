#ifndef CH_SCENE_H
#define CH_SCENE_H

#include <entt/entt.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "engine/core/events/events.h"
#include "engine/foundation/base.h"
#include "engine/foundation/timestep.h"
#include "engine/scene/entity.h"
#include "engine/scene/scene_settings.h"
#include "engine/scene/scene_state.h" // <--- Підключаємо наш enum SceneState

namespace Chained
{
class Physics;
class ScriptEngine;
class SceneScriptingManager;
class HierarchySystem;
class SceneResourceManager;
class AnimationManager;
class Event;

// Owns the scene registry, scene settings, and manages its own execution state lifecycle.
class CH_API Scene
{
public:
    Scene();
    ~Scene();

public:
    using EventCallbackFn = std::function<void(Event&)>;
    void SetEventCallback(const EventCallbackFn& callback)
    {
        m_EventCallback = callback;
    }

public:
    // Creates a new scene with default entities (e.g. Main Camera).
    static std::shared_ptr<Scene> CreateDefault();
    // Creates a deep copy of another scene.
    static std::shared_ptr<Scene> Copy(std::shared_ptr<Scene> other);

    virtual void OnEvent(Event& e);
    virtual void OnRenderUI();

public: // Керування станом сцени (State Management)
    void TransitionToState(SceneState newState);
    SceneState GetSceneState() const
    {
        return m_State;
    }

    // Єдина точка оновлення сцени (викликає потрібний під-апдейт залежно від m_State)
    void OnUpdate(Timestep timestep);
    void OnViewportResize(uint32_t width, uint32_t height);

public: // Entity Management
    Entity CreateEntity(const std::string& name = std::string());
    Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
    Entity CreateUIEntity(const std::string& type, const std::string& name = std::string());

public:
    entt::entity CopyEntity(entt::entity copyEntity);
    void DestroyEntity(Entity entity);
    Entity FindEntityByTag(const std::string& tag);
    Entity GetEntityByUUID(UUID uuid);

public:
    SceneSettings& GetSettings();
    const SceneSettings& GetSettings() const;
    bool IsSimulationRunning() const;
    std::vector<entt::entity> GetRootEntities();
    std::vector<entt::entity> GetRootEntities() const;

public: // Systems & Tools
    entt::registry& GetRegistry();
    const entt::registry& GetRegistry() const;
    entt::registry* GetRegistryPtr();

    void OnRuntimeStop();
    void OnRuntimeStart();
    void OnUpdateSimulation(Timestep timestep);
    void OnUpdateEditor(Timestep timestep);
    void OnUpdateRuntime(Timestep timestep);

private: // Внутрішні методи життєвого циклу станів
    void OnStateEnter(SceneState state);
    void OnStateExit(SceneState state);

private:
    SceneState m_State = SceneState::Edit;

    std::unique_ptr<entt::registry> m_Registry;
    SceneSettings m_Settings;

    std::unique_ptr<SceneScriptingManager> m_ScriptingManager;
    std::unique_ptr<HierarchySystem> m_HierarchySystem;
    std::unique_ptr<SceneResourceManager> m_ResourceManager;
    std::unique_ptr<AnimationManager> m_AnimationManager;
    EventCallbackFn m_EventCallback;

private:
    void OnIDConstruct(entt::registry& registry, entt::entity entity);
    void OnIDDestroy(entt::registry& registry, entt::entity entity);
    void OnHierarchyDestroy(entt::registry& registry, entt::entity entity);
    Entity CopyEntityInternal(entt::entity copyEntity, entt::entity parentEntity = entt::null);
};

} // namespace Chained

#endif // CH_SCENE_H
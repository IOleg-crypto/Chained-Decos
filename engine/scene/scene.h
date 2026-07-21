#ifndef CH_SCENE_H
#define CH_SCENE_H

#include <entt/entt.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "engine/core/events/events.h"
#include "engine/common/base.h"
#include "engine/common/timestep.h"
#include "engine/scene/entity.h"
#include "engine/scene/scene_context.h"
#include "engine/scene/scene_settings.h"
#include "engine/scene/scene_state.h"

namespace Chained
{
class Physics;
class ScriptEngine;
class SceneScriptingManager;
class Event;

/// @brief Owns the scene registry, scene settings, and manages its own execution state lifecycle.
///
/// Scenes are the primary container for ECS entities and systems. They manage
/// state transitions between Edit, Runtime, and Simulation modes, and coordinate
/// script execution, physics, animation, and scene transitions.
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
    /// @brief Create a new scene with default entities (Main Camera).
    static std::shared_ptr<Scene> CreateDefault();

    /// @brief Deep-copy another scene, duplicating all entities and components.
    static std::shared_ptr<Scene> Copy(std::shared_ptr<Scene> other);

    virtual void OnEvent(Event& e);
    virtual void OnRenderUI();

public: // Scene State Management
    /// @brief Transition the scene to a new state (Edit, Runtime, Simulation).
    /// Calls OnStateExit for the current state and OnStateEnter for the new one.
    void TransitionToState(SceneState newState, const SceneContext& ctx);
    SceneState GetSceneState() const
    {
        return m_State;
    }

    /// @brief Single update entry point — dispatches to the correct sub-update based on current state.
    void OnUpdate(Timestep timestep, const SceneContext& ctx);
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

    void OnRuntimeStop(const SceneContext& ctx);
    void OnRuntimeStart(const SceneContext& ctx);
    void OnUpdateSimulation(Timestep timestep, const SceneContext& ctx);
    void OnUpdateEditor(Timestep timestep, const SceneContext& ctx);

    /// @brief Runtime update — runs scripts, physics, animations, and scene transitions.
    void OnUpdateRuntime(Timestep timestep, const SceneContext& ctx);

private: // Internal state lifecycle methods
    void OnStateEnter(SceneState state, const SceneContext& ctx);
    void OnStateExit(SceneState state, const SceneContext& ctx);

private:
    SceneState m_State = SceneState::Edit;

    std::unique_ptr<entt::registry> m_Registry;
    SceneSettings m_Settings;

    std::unique_ptr<SceneScriptingManager> m_ScriptingManager;
    EventCallbackFn m_EventCallback;

private:
    void OnIDConstruct(entt::registry& registry, entt::entity entity);
    void OnIDDestroy(entt::registry& registry, entt::entity entity);
    void OnHierarchyDestroy(entt::registry& registry, entt::entity entity);
    Entity CopyEntityInternal(entt::entity copyEntity, entt::entity parentEntity = entt::null);
    std::vector<entt::entity> GetRootEntitiesImpl() const;
};

} // namespace Chained

#endif // CH_SCENE_H
#ifndef CH_SCENE_H
#define CH_SCENE_H

#include "components.h"
#include "engine/core/base.h"
#include "engine/graphics/environment.h"
#include "entity.h"
#include "entt/entt.hpp"
#include "imgui.h"
#include <string>

#include "engine/core/events.h"
#include "engine/physics/physics.h"

namespace CHEngine
{

enum class BackgroundMode : uint8_t
{
    Color,
    Texture,
    Environment3D
};

struct SceneSettings
{
    BackgroundMode Mode = BackgroundMode::Environment3D;
    Color BackgroundColor;
    std::string BackgroundTexturePath;
    std::string ScenePath;
    CanvasSettings Canvas;
    std::shared_ptr<EnvironmentAsset> Environment;
};

class Scene
{
public:
    Scene();
    ~Scene();

    static std::shared_ptr<Scene> Copy(std::shared_ptr<Scene> other);

public: // Entity Management
    Entity CreateEntity(const std::string &name = "Entity");
    Entity CreateEntityWithUUID(UUID uuid, const std::string &name = "Entity");
    Entity CreateUIEntity(const std::string &type, const std::string &name = "");
    void DestroyEntity(Entity entity);

    Entity FindEntityByTag(const std::string &tag);
    Entity GetEntityByUUID(UUID uuid);

public: // Runtime & Simulation
    void OnRuntimeStart();
    void OnRuntimeStop();
    void OnUpdateRuntime(float deltaTime);
    void OnRender(const Camera3D &camera, Timestep ts = 0, const struct DebugRenderFlags *debugFlags = nullptr);
    
    bool IsSimulationRunning() const;
    void OnEvent(Event &e);
    
    void RequestSceneChange(const std::string &path);
    void UpdateProfilerStats();

public: // Scene Settings & Environment
    BackgroundMode GetBackgroundMode() const;
    void SetBackgroundMode(BackgroundMode mode);

    Color GetBackgroundColor() const;
    void SetBackgroundColor(Color color);

    const std::string& GetBackgroundTexturePath() const;
    void SetBackgroundTexturePath(const std::string& path);

    const std::string& GetScenePath() const;
    void SetScenePath(const std::string& path);

    std::shared_ptr<EnvironmentAsset> GetEnvironment();
    const std::shared_ptr<EnvironmentAsset> GetEnvironment() const;
    void SetEnvironment(std::shared_ptr<EnvironmentAsset> environment);

    CanvasSettings& GetCanvasSettings();
    const CanvasSettings& GetCanvasSettings() const;

    Physics& GetPhysics() { return *m_Physics; }
    const Physics& GetPhysics() const { return *m_Physics; }

public: // Systems & Tools
    void OnImGuiRender(const ImVec2 &refPos = {0, 0}, const ImVec2 &refSize = {0, 0},
                       uint32_t viewportID = 0, bool editMode = false);

    entt::registry &GetRegistry() { return m_Registry; }
    const entt::registry &GetRegistry() const { return m_Registry; }

private:
    entt::registry m_Registry;
    std::unordered_map<UUID, entt::entity> m_EntityMap;
    SceneSettings m_Settings;
    std::unique_ptr<Physics> m_Physics;

    bool m_IsSimulationRunning = false;

private: // Internal Event Handlers
    // Reactive signals handlers
    void OnModelComponentAdded(entt::registry &reg, entt::entity entity);
    void OnAnimationComponentAdded(entt::registry &reg, entt::entity entity);
    void OnAudioComponentAdded(entt::registry &reg, entt::entity entity);

    // UUID Map Handlers
    void OnIDConstruct(entt::registry &reg, entt::entity entity);
    void OnIDDestroy(entt::registry &reg, entt::entity entity);

    // Hierarchy Handlers
    void OnHierarchyDestroy(entt::registry &reg, entt::entity entity);

    // Generic component added handling (templated)
    template <typename T> void OnComponentAdded(Entity entity, T &component) {}

    friend class Entity;
    friend class SceneSerializer;
};

// Template specializations (must be in namespace scope)
template <> void Scene::OnComponentAdded<ModelComponent>(Entity entity, ModelComponent &component);
template <>
void Scene::OnComponentAdded<AnimationComponent>(Entity entity, AnimationComponent &component);
template <> void Scene::OnComponentAdded<AudioComponent>(Entity entity, AudioComponent &component);

// Entity Template Implementations
template <typename T, typename... Args> T &Entity::AddComponent(Args &&...args)
{
    CH_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
    T &component = m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
    return component;
}



template <typename T> T &Entity::GetComponent()
{
    return m_Scene->m_Registry.get<T>(m_EntityHandle);
}

template <typename T> bool Entity::HasComponent()
{
    return m_Scene->m_Registry.all_of<T>(m_EntityHandle);
}

template <typename T> void Entity::RemoveComponent()
{
    m_Scene->m_Registry.remove<T>(m_EntityHandle);
}

template <typename T, typename... Func> void Entity::Patch(Func &&...func)
{
    m_Scene->m_Registry.patch<T>(m_EntityHandle, std::forward<Func>(func)...);
}

} // namespace CHEngine

#endif // CH_SCENE_H

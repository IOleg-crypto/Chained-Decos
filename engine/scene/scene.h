#ifndef CH_SCENE_H
#define CH_SCENE_H

#include "components.h"
#include "engine/core/base.h"
#include "engine/graphics/environment.h"
#include "entity.h"
#include "entt/entt.hpp"
#include "imgui.h"
#include "string"

#include "engine/core/events.h"

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
    SkyboxComponent Skybox;
};

class Scene
{
public:
    Scene();
    virtual ~Scene();

    BackgroundMode GetBackgroundMode() const
    {
        return m_Settings.Mode;
    }
    void SetBackgroundMode(BackgroundMode mode)
    {
        m_Settings.Mode = mode;
    }

    Color GetBackgroundColor() const
    {
        return m_Settings.BackgroundColor;
    }
    void SetBackgroundColor(Color color)
    {
        m_Settings.BackgroundColor = color;
    }

    const std::string &GetBackgroundTexturePath() const
    {
        return m_Settings.BackgroundTexturePath;
    }
    void SetBackgroundTexturePath(const std::string &path)
    {
        m_Settings.BackgroundTexturePath = path;
    }

    static std::shared_ptr<Scene> Copy(std::shared_ptr<Scene> other);

    Entity CreateEntity(const std::string &name = "Entity");
    Entity CreateEntityWithUUID(UUID uuid, const std::string &name = "Entity");
    Entity CreateUIEntity(const std::string &type, const std::string &name = "");
    void DestroyEntity(Entity entity);

    Entity FindEntityByTag(const std::string &tag);
    Entity GetEntityByUUID(UUID uuid);

    void OnUpdateRuntime(float deltaTime);
    void OnUpdateEditor(float deltaTime);
    void OnRender(const Camera3D &camera, const struct DebugRenderFlags *debugFlags = nullptr);
    void OnRuntimeStart();
    void OnRuntimeStop();
    bool IsSimulationRunning() const
    {
        return m_IsSimulationRunning;
    }
    void OnEvent(Event &e);
    void OnImGuiRender(const ImVec2 &refPos = {0, 0}, const ImVec2 &refSize = {0, 0},
                       uint32_t viewportID = 0, bool editMode = false);

    void RequestSceneChange(const std::string &path);
    void UpdateProfilerStats();

    const std::string &GetScenePath() const
    {
        return m_Settings.ScenePath;
    }
    void SetScenePath(const std::string &path)
    {
        m_Settings.ScenePath = path;
    }

    // Generic component added handling (templated)
    template <typename T> void OnComponentAdded(Entity entity, T &component)
    {
    }

    entt::registry &GetRegistry()
    {
        return m_Registry;
    }
    const entt::registry &GetRegistry() const
    {
        return m_Registry;
    }

    struct SkyboxComponent &GetSkybox()
    {
        return m_Settings.Skybox;
    }
    const struct SkyboxComponent &GetSkybox() const
    {
        return m_Settings.Skybox;
    }

    std::shared_ptr<EnvironmentAsset> GetEnvironment()
    {
        return m_Settings.Environment;
    }
    const std::shared_ptr<EnvironmentAsset> GetEnvironment() const
    {
        return m_Settings.Environment;
    }
    void SetEnvironment(std::shared_ptr<EnvironmentAsset> environment)
    {
        m_Settings.Environment = environment;
    }

    Camera3D GetActiveCamera() const;
    EnvironmentSettings GetEnvironmentSettings() const;

    CanvasSettings &GetCanvasSettings()
    {
        return m_Settings.Canvas;
    }
    const CanvasSettings &GetCanvasSettings() const
    {
        return m_Settings.Canvas;
    }

private:
    entt::registry m_Registry;
    std::unordered_map<UUID, entt::entity> m_EntityMap;
    SceneSettings m_Settings;

    bool m_IsSimulationRunning = false;

    // Declarative internal pipeline
    void UpdateScripting(float deltaTime);
    void UpdateAnimation(float deltaTime);
    void UpdateAudio(float deltaTime);

    // Reactive signals handlers
    void OnModelComponentAdded(entt::registry &reg, entt::entity entity);
    void OnAnimationComponentAdded(entt::registry &reg, entt::entity entity);
    void OnAudioComponentAdded(entt::registry &reg, entt::entity entity);

    // UUID Map Handlers
    void OnIDConstruct(entt::registry &reg, entt::entity entity);
    void OnIDDestroy(entt::registry &reg, entt::entity entity);

    // Hierarchy Handlers
    void OnHierarchyDestroy(entt::registry &reg, entt::entity entity);

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

inline bool Entity::IsValid() const
{
    return m_Scene && m_Scene->m_Registry.valid(m_EntityHandle);
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

#ifndef CH_SCENE_H
#define CH_SCENE_H

#include "components.h"
#include "engine/core/base.h"
#include "engine/render/environment.h"
#include "entity.h"
#include <entt/entt.hpp>
#include <imgui.h>
#include <string>

#include "engine/core/events.h"

namespace CHEngine
{

enum class BackgroundMode : uint8_t
{
    Color,
    Texture,
    Environment3D
};

class Scene
{
public:
    Scene();
    virtual ~Scene();

    BackgroundMode GetBackgroundMode() const
    {
        return m_BackgroundMode;
    }
    void SetBackgroundMode(BackgroundMode mode)
    {
        m_BackgroundMode = mode;
    }

    Color GetBackgroundColor() const
    {
        return m_BackgroundColor;
    }
    void SetBackgroundColor(Color color)
    {
        m_BackgroundColor = color;
    }

    const std::string &GetBackgroundTexturePath() const
    {
        return m_BackgroundTexturePath;
    }
    void SetBackgroundTexturePath(const std::string &path)
    {
        m_BackgroundTexturePath = path;
    }

    static std::shared_ptr<Scene> Copy(std::shared_ptr<Scene> other);

    Entity CreateEntity(const std::string &name = "Entity");
    Entity CreateEntityWithUUID(UUID uuid, const std::string &name = "Entity");
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
        return m_ScenePath;
    }
    void SetScenePath(const std::string &path)
    {
        m_ScenePath = path;
    }

    // Entt-compatible wrappers
    void OnModelComponentAdded(entt::registry &reg, entt::entity entity);
    void OnAnimationComponentAdded(entt::registry &reg, entt::entity entity);
    void OnAudioComponentAdded(entt::registry &reg, entt::entity entity);

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
        return m_Skybox;
    }
    const struct SkyboxComponent &GetSkybox() const
    {
        return m_Skybox;
    }

    std::shared_ptr<EnvironmentAsset> GetEnvironment()
    {
        return m_Environment;
    }
    const std::shared_ptr<EnvironmentAsset> GetEnvironment() const
    {
        return m_Environment;
    }
    void SetEnvironment(std::shared_ptr<EnvironmentAsset> environment)
    {
        m_Environment = environment;
    }

    const Camera3D &GetActiveCamera() const
    {
        return m_ActiveCamera;
    }
    EnvironmentSettings GetEnvironmentSettings() const;

private:
    entt::registry m_Registry;
    std::shared_ptr<EnvironmentAsset> m_Environment;
    struct SkyboxComponent m_Skybox;

    BackgroundMode m_BackgroundMode = BackgroundMode::Environment3D;
    Color m_BackgroundColor = {245, 245, 245, 255};
    std::string m_BackgroundTexturePath = "";
    std::string m_ScenePath = "";

    bool m_IsSimulationRunning = false;

    friend class Entity;
    friend class SceneSerializer;

    Camera3D m_ActiveCamera;
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
    m_Scene->OnComponentAdded<T>(*this, component);
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

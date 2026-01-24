#ifndef CH_SCENE_H
#define CH_SCENE_H

#include "components.h"
#include "engine/core/base.h"
#include "engine/renderer/environment.h"
#include "entity.h"
#include <entt/entt.hpp>
#include <imgui.h>
#include <string>

#include "engine/core/events.h"

namespace CHEngine
{
class SceneScripting;
class SceneAnimator;
class SceneAudio;

class Scene
{
public:
    Scene();
    virtual ~Scene();

    static Ref<Scene> Copy(Ref<Scene> other);

    Entity CreateEntity(const std::string &name = "Entity");
    Entity CreateEntityWithUUID(UUID uuid, const std::string &name = "Entity");
    void DestroyEntity(Entity entity);

    Entity FindEntityByTag(const std::string &tag);
    Entity GetEntityByUUID(UUID uuid);

    void OnUpdateRuntime(float deltaTime);
    void OnUpdateEditor(float deltaTime);
    void OnRuntimeStart();
    void OnRuntimeStop();
    bool IsSimulationRunning() const
    {
        return m_IsSimulationRunning;
    }
    void OnEvent(Event &e);

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

    Ref<EnvironmentAsset> GetEnvironment()
    {
        return m_Environment;
    }
    const Ref<EnvironmentAsset> GetEnvironment() const
    {
        return m_Environment;
    }
    void SetEnvironment(Ref<EnvironmentAsset> env)
    {
        m_Environment = env;
    }

private:
    entt::registry m_Registry;
    Ref<EnvironmentAsset> m_Environment;
    struct SkyboxComponent m_Skybox;
    bool m_IsSimulationRunning = false;

    Scope<SceneScripting> m_Scripting;
    Scope<SceneAnimator> m_Animator;
    Scope<SceneAudio> m_Audio;

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
    return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
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

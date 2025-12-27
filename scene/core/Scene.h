#ifndef SCENE_H
#define SCENE_H

#include <entt/entt.hpp>
#include <string>

namespace CHEngine
{

class Entity; // Forward declaration

/**
 * Scene - Core ECS container
 *
 * Manages entities and their components using EnTT registry.
 * Each scene has its own isolated ECS world.
 */
class Scene
{
public:
    Scene(const std::string &name = "Untitled");
    ~Scene() = default;

    // Entity Management
    Entity CreateEntity(const std::string &name = "Entity");
    Entity CreateEntityWithUUID(uint64_t uuid, const std::string &name = "Entity");
    void DestroyEntity(Entity entity);
    void DestroyEntity(entt::entity entity);

    // Lifecycle
    void OnUpdateRuntime(float deltaTime);
    void OnUpdateEditor(float deltaTime);
    void OnRenderRuntime();
    void OnRenderEditor();

    // Accessors
    entt::registry &GetRegistry()
    {
        return m_Registry;
    }
    const entt::registry &GetRegistry() const
    {
        return m_Registry;
    }

    const std::string &GetName() const
    {
        return m_Name;
    }
    void SetName(const std::string &name)
    {
        m_Name = name;
    }

    uint32_t GetViewportWidth() const
    {
        return m_ViewportWidth;
    }
    uint32_t GetViewportHeight() const
    {
        return m_ViewportHeight;
    }
    void SetViewportSize(uint32_t width, uint32_t height);

private:
    entt::registry m_Registry;
    std::string m_Name;

    uint32_t m_ViewportWidth = 1280;
    uint32_t m_ViewportHeight = 720;

    friend class Entity;
    friend class ECSSceneSerializer;
};

} // namespace CHEngine

#endif // SCENE_H

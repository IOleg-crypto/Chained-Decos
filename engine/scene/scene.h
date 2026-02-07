#ifndef CH_SCENE_H
#define CH_SCENE_H

#include "components.h"
#include "engine/core/base.h"
#include "engine/core/assert.h"
#include "engine/core/events.h"
#include "engine/core/timestep.h"
#include "engine/graphics/environment.h"
#include "entt/entt.hpp"
#include <string>
#include <unordered_map>
#include <memory>

namespace CHEngine
{
    class Scene;
    class SceneSerializer;
    class Physics;

    enum class BackgroundMode
    {
        Color = 0,
        Texture = 1,
        Environment3D = 2
    };

    struct SceneSettings
    {
        std::string Name = "Untitled Scene";
        std::string ScenePath;
        std::shared_ptr<EnvironmentAsset> Environment;
        
        BackgroundMode Mode = BackgroundMode::Environment3D;
        Color BackgroundColor = {30, 30, 30, 255};
        std::string BackgroundTexturePath;

        CanvasSettings Canvas;

        bool GravityEnabled = true;
        Vector3 Gravity = {0, -9.81f, 0};
        
        float AmbientIntensity = 0.5f;
    };

    class Entity
    {
    public:
        Entity() = default;
        Entity(entt::entity handle, Scene* scene) : m_EntityHandle(handle), m_Scene(scene) {}
        Entity(const Entity& other) = default;

        template <typename T, typename... Args> T& AddOrReplaceComponent(Args&&... args);
        template <typename T, typename... Args> T& AddComponent(Args&&... args);

        template <typename T> T& GetComponent();
        template <typename T> bool HasComponent();
        template <typename T> void RemoveComponent();
        template <typename T, typename... Func> void Patch(Func&&... func);

        operator bool() const { return m_EntityHandle != entt::null && m_Scene != nullptr; }
        bool IsValid() const;
        operator entt::entity() const { return m_EntityHandle; }
        operator uint32_t() const { return (uint32_t)m_EntityHandle; }

        bool operator==(const Entity& other) const
        {
            return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
        }
        bool operator!=(const Entity& other) const
        {
            return !(*this == other);
        }

        Scene* GetScene() { return m_Scene; }
        const Scene* GetScene() const { return m_Scene; }

        UUID GetUUID();
        const std::string& GetName();

    private:
        entt::entity m_EntityHandle{ entt::null };
        Scene* m_Scene = nullptr;
    };

    class Scene : public std::enable_shared_from_this<Scene>
    {
    public:
        Scene();
        ~Scene();

        static std::shared_ptr<Scene> Copy(std::shared_ptr<Scene> other);

        Entity CreateEntity(const std::string& name = std::string());
        Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
        Entity CreateUIEntity(const std::string& type, const std::string& name = std::string());
        void DestroyEntity(Entity entity);

        Entity FindEntityByTag(const std::string& tag);
        Entity GetEntityByUUID(UUID uuid);

    public: // Life Cycle & Simulation
        void OnRuntimeStart();
        void OnRuntimeStop();
        void OnUpdateRuntime(Timestep timestep);
        void OnViewportResize(uint32_t width, uint32_t height);
        
        bool IsSimulationRunning() const { return m_IsSimulationRunning; }
        void OnEvent(Event& event);

    public: // Scene Settings
        SceneSettings& GetSettings() { return m_Settings; }
        const SceneSettings& GetSettings() const { return m_Settings; }

        Physics& GetPhysics() { return *m_Physics; }
        const Physics& GetPhysics() const { return *m_Physics; }

    public: // Systems & Tools
        entt::registry& GetRegistry() { return m_Registry; }
        const entt::registry& GetRegistry() const { return m_Registry; }

        Camera3D GetActiveCamera();

    private:
        entt::registry m_Registry;
        std::unordered_map<UUID, entt::entity> m_EntityMap;
        SceneSettings m_Settings;
        std::unique_ptr<Physics> m_Physics;

        bool m_IsSimulationRunning = false;

    private: // Internal Event Handlers
        // Reactive signals handlers
        void OnModelComponentAdded(entt::registry& registry, entt::entity entity);
        void OnAnimationComponentAdded(entt::registry& registry, entt::entity entity);
        void OnAudioComponentAdded(entt::registry& registry, entt::entity entity);
        void OnColliderComponentAdded(entt::registry& registry, entt::entity entity);
        void OnPanelControlAdded(entt::registry& registry, entt::entity entity);

        void OnIDConstruct(entt::registry& registry, entt::entity entity);
        void OnIDDestroy(entt::registry& registry, entt::entity entity);

        // Hierarchy Handlers
        void OnHierarchyDestroy(entt::registry& registry, entt::entity entity);
        
    private: // Update Logic
        void UpdatePhysics(float deltaTime);
        void UpdateAnimations(float deltaTime);
        void UpdateScripting(float deltaTime);
        void UpdateAudio(float deltaTime);
        void UpdateTransitions();

        friend class Entity;
        friend class SceneSerializer;
    };

    // Entity Template Implementations
    template <typename T, typename... Args> T &Entity::AddComponent(Args &&...args)
    {
        CH_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
        T &component = m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
        return component;
    }

    template <typename T, typename... Args> T &Entity::AddOrReplaceComponent(Args &&...args)
    {
        T &component = m_Scene->m_Registry.emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
        return component;
    }

    template <typename T> T &Entity::GetComponent()
    {
        CH_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
        return m_Scene->m_Registry.get<T>(m_EntityHandle);
    }

    template <typename T> bool Entity::HasComponent()
    {
        return m_Scene->m_Registry.all_of<T>(m_EntityHandle);
    }

    template <typename T> void Entity::RemoveComponent()
    {
        CH_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
        m_Scene->m_Registry.remove<T>(m_EntityHandle);
    }

    template <typename T, typename... Func> void Entity::Patch(Func &&...func)
    {
        m_Scene->m_Registry.patch<T>(m_EntityHandle, std::forward<Func>(func)...);
    }
} // namespace CHEngine

#endif // CH_SCENE_H

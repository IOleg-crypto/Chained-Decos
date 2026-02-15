#ifndef CH_SCENE_H
#define CH_SCENE_H

#include "components.h"
#include "engine/core/base.h"
#include "engine/core/assert.h"
#include "engine/core/events.h"
#include "engine/core/timestep.h"
#include "engine/scene/scene_settings.h"
#include "engine/scene/entity.h"
#include "entt/entt.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <raylib.h>

namespace CHEngine
{
    class SceneSerializer;
    class Physics;
    class ScriptRegistry;

    class Scene : public std::enable_shared_from_this<Scene>
    {
    public:
        Scene();
        ~Scene();

        static std::shared_ptr<Scene> Copy(std::shared_ptr<Scene> other);

        Entity CreateEntity(const std::string& name = std::string());
        Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
        Entity CreateUIEntity(const std::string& type, const std::string& name = std::string());
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
        
        bool IsSimulationRunning() const { return m_IsSimulationRunning; }
        void OnEvent(Event& event);

    public: // Scene Settings
        SceneSettings& GetSettings() { return m_Settings; }
        const SceneSettings& GetSettings() const { return m_Settings; }

        Physics& GetPhysics() { return *m_Physics; }
        const Physics& GetPhysics() const { return *m_Physics; }

        ScriptRegistry& GetScriptRegistry() { return *m_ScriptRegistry; }
        const ScriptRegistry& GetScriptRegistry() const { return *m_ScriptRegistry; }

    public: // Systems & Tools
        entt::registry& GetRegistry() { return m_Registry; }
        const entt::registry& GetRegistry() const { return m_Registry; }

        std::optional<Camera3D> GetActiveCamera();
        Entity GetPrimaryCameraEntity();

    private:
        Camera3D GetCameraFromEntity(entt::entity entityHandle);

    private:
        entt::registry m_Registry;
        std::unordered_map<UUID, entt::entity> m_EntityMap;
        SceneSettings m_Settings;
        std::unique_ptr<Physics> m_Physics;
        std::unique_ptr<ScriptRegistry> m_ScriptRegistry;

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
        void UpdatePhysics(Timestep deltaTime);
        void UpdateAnimations(Timestep deltaTime);
        void UpdateScripting(Timestep deltaTime);
        void UpdateAudio(Timestep deltaTime);
        void UpdateCameras(Timestep deltaTime);
        void UpdateTransitions();

        friend class Entity;
        friend class SceneSerializer;
    };

} // namespace CHEngine

#endif // CH_SCENE_H

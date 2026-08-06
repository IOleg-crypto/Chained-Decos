#ifndef CH_SCENE_H
#define CH_SCENE_H

#include <entt/entt.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <future>

#include "engine/core/events/events.h"
#include "engine/common/base.h"
#include "engine/common/timestep.h"
#include "engine/scene/entity.h"
#include "engine/scene/scene_settings.h"
#include "engine/scene/scene_state.h"

namespace Chained
{
	class SceneScriptingManager;
	class Event;

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
		static std::shared_ptr<Scene> CreateDefault();
		static std::shared_ptr<Scene> Copy(std::shared_ptr<Scene> other);

		virtual void OnEvent(Event& e);
		virtual void OnRenderUI();

	public: // Scene State Management
		void TransitionToState(SceneState newState);
		SceneState GetSceneState() const
		{
			return m_State;
		}

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
		const std::vector<entt::entity>& GetRootEntities() const;
		bool IsStartingUp() const
		{
			return m_IsStartingUp;
		}

		// Validation setters — prefer these over direct GetSettings() mutation
		void SetSceneName(const std::string& name)
		{
			m_Settings.Name = name;
		}
		void SetScenePath(const std::string& path)
		{
			m_Settings.ScenePath = path;
		}
		void SetEnvironment(std::shared_ptr<EnvironmentAsset> env)
		{
			m_Settings.Environment = std::move(env);
		}
		void SetBackgroundMode(BackgroundMode mode)
		{
			m_Settings.Mode = mode;
		}
		void SetBackgroundColor(const Color& color)
		{
			m_Settings.BackgroundColor = color;
		}
		void SetDebugFlags(const DebugRenderFlags& flags)
		{
			m_Settings.DebugFlags = flags;
		}
		void SetDiagnosticMode(float mode)
		{
			m_Settings.DiagnosticMode = std::clamp(mode, 0.0f, 2.0f);
		}
		void SetGridSlices(int slices)
		{
			m_Settings.Grid.Slices = std::max(1, slices);
		}
		void SetGridSpacing(float spacing)
		{
			m_Settings.Grid.Spacing = std::max(0.01f, spacing);
		}

	public: // Systems & Tools
		/// Returns the underlying EnTT registry for direct access.
		/// @warning Returns mutable reference — prefer Entity API for component access.
		/// Direct registry manipulation bypasses Entity lifecycle hooks.
		entt::registry& GetRegistry();
		const entt::registry& GetRegistry() const;
		entt::registry* GetRegistryPtr();

		void OnRuntimeStop();
		void OnRuntimeStart();
		void OnUpdateSimulation(Timestep timestep);
		void OnUpdateEditor(Timestep timestep);
		void OnUpdateRuntime(Timestep timestep);

	private:
		void UpdateCommon(Timestep ts, bool runScripting, bool runPhysics, bool runTransitions);
		void InitializePhysicsStartup();
		void RebuildRootCache() const;

	private:
		void OnStateEnter(SceneState state);
		void OnStateExit(SceneState state);
		void FinishRuntimeStart();

	private:
		SceneState m_State = SceneState::Edit;

		std::unique_ptr<entt::registry> m_Registry;
		SceneSettings m_Settings;

		std::unique_ptr<SceneScriptingManager> m_ScriptingManager;
		EventCallbackFn m_EventCallback;

		bool m_IsStartingUp = false;

		mutable std::vector<entt::entity> m_CachedRoots;
		mutable bool m_RootsDirty = true;

	private:
		void OnIDConstruct(entt::registry& registry, entt::entity entity);
		void OnIDDestroy(entt::registry& registry, entt::entity entity);
		void OnHierarchyConstruct();
		void OnHierarchyDestroy(entt::registry& registry, entt::entity entity);
		Entity CopyEntityInternal(entt::entity copyEntity, entt::entity parentEntity = entt::null);
	};

} // namespace Chained

#endif // CH_SCENE_H

#include "scene_scripting_manager.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"
#include "engine/physics/physics.h"
#include "engine/scene/scene.h"
#include "scripting/scriptengine.h"
#include "scripting/scriptengine_services.h"
#include "scripting/script_glue_internal.h"
#include "scripting/script_interop_pointers.h"
#include <Coral/String.hpp>
#include <Coral/Type.hpp>

namespace Chained
{

	static std::vector<SceneScriptingManager*> s_Managers;
	static std::mutex s_ManagersMutex;

	void SceneScriptingManager::Register(SceneScriptingManager* manager)
	{
		std::lock_guard<std::mutex> lock(s_ManagersMutex);
		s_Managers.push_back(manager);
	}

	void SceneScriptingManager::Unregister(SceneScriptingManager* manager)
	{
		std::lock_guard<std::mutex> lock(s_ManagersMutex);
		auto it = std::find(s_Managers.begin(), s_Managers.end(), manager);
		if (it != s_Managers.end())
		{
			s_Managers.erase(it);
		}
	}

	void SceneScriptingManager::ResetAll()
	{
		// Snapshot under lock, then iterate without holding it.
		// OnRuntimeStop() may call Unregister() which also takes s_ManagersMutex —
		// iterating while holding the lock would cause a deadlock.
		std::vector<SceneScriptingManager*> snapshot;
		{
			std::lock_guard<std::mutex> lock(s_ManagersMutex);
			snapshot = s_Managers;
		}
		for (auto* manager : snapshot)
		{
			manager->OnRuntimeStop();
		}
	}

	SceneScriptingManager::SceneScriptingManager(Scene* scene)
		: m_Scene(scene)
	{
		SceneScriptingManager::Register(this);
	}

	SceneScriptingManager::ScriptEngineContext SceneScriptingManager::AcquireScriptEngine()
	{
		ScriptEngineContext ctx;
		ctx.engine = ServiceLocator::TryGet<ScriptEngine>();
		if (!ctx.engine || !ctx.engine->GetHost().IsInitialized())
		{
			return {};
		}
		auto* coreAssembly = ctx.engine->GetHost().GetCoreAssembly();
		if (!coreAssembly)
		{
			return {};
		}
		ctx.scriptEngineType = coreAssembly->GetLocalType("Chained.ScriptEngine");
		return ctx;
	}

	SceneScriptingManager::~SceneScriptingManager()
	{
		SceneScriptingManager::Unregister(this);

		if (m_Scene && ServiceLocator::IsAvailable())
		{
			if (m_Scene->IsSimulationRunning())
			{
				auto ctx = AcquireScriptEngine();
				if (ctx.engine && ctx.scriptEngineType)
				{
					if (ManagedCallbacks_::ClearAll)
					{
						ManagedCallbacks_::ClearAll();
					}
				}
			}
		}
	}

	void SceneScriptingManager::OnRuntimeStart()
	{
		CH_CORE_INFO("SceneScriptingManager::OnRuntimeStart - C# ScriptEngine based (Delegated)");
		if (!m_Scene)
		{
			return;
		}

		auto& registry = m_Scene->GetRegistry();
		auto view = registry.view<ManagedScriptComponent>();
		for (auto entity : view)
		{
			auto& msc = registry.get<Chained::ManagedScriptComponent>(entity);
			for (auto& script : msc.Scripts)
			{
				script.IsInstantiated = false;
				script.NeedsStart = true;
			}
		}

		entt::registry* registryPtr = m_Scene->GetRegistryPtr();
	}

	void SceneScriptingManager::OnRuntimeStop()
	{
		auto ctx = AcquireScriptEngine();
		if (ctx.engine && ctx.scriptEngineType && !m_ReloadInProgress)
		{
			if (ManagedCallbacks_::ClearAll)
			{
				ManagedCallbacks_::ClearAll();
			}
		}

		auto& registry = m_Scene->GetRegistry();
		auto view = registry.view<ManagedScriptComponent>();
		for (auto entity : view)
		{
			auto& msc = registry.get<Chained::ManagedScriptComponent>(entity);
			for (auto& script : msc.Scripts)
			{
				script.IsInstantiated = false;
				script.NeedsStart = true;
			}
		}
	}

	void SceneScriptingManager::OnUpdate(Timestep deltaTime)
	{
		CH_PROFILE_FUNCTION();

		if (!m_Scene || !m_Scene->IsSimulationRunning())
		{
			return;
		}

		auto ctx = AcquireScriptEngine();
		if (!ctx.engine || !ctx.scriptEngineType)
		{
			return;
		}

		ctx.engine->SetContextScene(m_Scene);

		auto& registry = m_Scene->GetRegistry();
		auto view = registry.view<ManagedScriptComponent>();
		for (auto entity : view)
		{
			auto& msc = registry.get<Chained::ManagedScriptComponent>(entity);
			for (auto& script : msc.Scripts)
			{
				if (!script.HasInstance() && !script.ClassName.empty())
				{
					try
					{
						CH_CORE_INFO("C++ calling InstantiateScript for {}", script.ClassName);
						std::u16string classNameStr = ch_utf8_to_u16(script.ClassName);
						if (ManagedCallbacks_::InstantiateScript)
						{
							ManagedCallbacks_::InstantiateScript(static_cast<uint64_t>(entity), classNameStr.c_str());
						}
						CH_CORE_INFO("C++ calling InstantiateScript SUCCESS");

						for (const auto& [fieldName, field] : script.Fields)
						{
							CH_CORE_INFO("C++ setting field {}", fieldName);
							Coral::String fNameStr = Coral::String::New(fieldName);
							Coral::String cNameStr = Coral::String::New(script.ClassName);
							if (field.Type == ScriptFieldType::Float)
							{
								if (auto* val = std::get_if<float>(&field.Value))
								{
									ctx.scriptEngineType.InvokeStaticMethod(
										"SetFieldFloat", static_cast<uint64_t>(entity), cNameStr, fNameStr, *val);
								}
							}
							else if (field.Type == ScriptFieldType::Int)
							{
								if (auto* val = std::get_if<int>(&field.Value))
								{
									ctx.scriptEngineType.InvokeStaticMethod(
										"SetFieldInt", static_cast<uint64_t>(entity), cNameStr, fNameStr, *val);
								}
							}
							else if (field.Type == ScriptFieldType::Bool)
							{
								if (auto* val = std::get_if<bool>(&field.Value))
								{
									ctx.scriptEngineType.InvokeStaticMethod(
										"SetFieldBool", static_cast<uint64_t>(entity), cNameStr, fNameStr, *val);
								}
							}
							else if (field.Type == ScriptFieldType::String)
							{
								if (auto* val = std::get_if<std::string>(&field.Value))
								{
									Coral::String vStr = Coral::String::New(*val);
									ctx.scriptEngineType.InvokeStaticMethod(
										"SetFieldString", static_cast<uint64_t>(entity), cNameStr, fNameStr, vStr);
									Coral::String::Free(vStr);
								}
							}
							Coral::String::Free(cNameStr);
							Coral::String::Free(fNameStr);
						}

						// Mark instantiated on C++ side
						script.IsInstantiated = true;
						script.NeedsStart = false; // Start is called natively on the C# side
					} catch (const std::exception& e)
					{
						CH_CORE_ERROR("ScriptEngine: Exception instantiating '{}': {}", script.ClassName, e.what());
					}
				}
			}
		}

		try
		{
			if (ManagedCallbacks_::OnUpdate)
			{
				ManagedCallbacks_::OnUpdate((float)deltaTime);
			}
		} catch (const std::exception& e)
		{
			CH_CORE_ERROR("ScriptEngine: Exception in OnUpdate Loop: {}", e.what());
		}
	}

	void SceneScriptingManager::OnEvent(Event& e)
	{
		if (m_ReloadInProgress || !m_Scene || e.GetEventType() == EventType::None)
		{
			return;
		}

		auto ctx = AcquireScriptEngine();
		if (!ctx.engine)
		{
			return;
		}

		ctx.engine->SetContextScene(m_Scene);

		if (ctx.scriptEngineType)
		{
			if (ManagedCallbacks_::OnEvent)
			{
				ManagedCallbacks_::OnEvent((int)e.GetEventType());
			}
		}

		ctx.engine->SetContextScene(nullptr);
	}

	void SceneScriptingManager::OnRenderUI()
	{
		if (m_ReloadInProgress)
		{
			return;
		}

		auto ctx = AcquireScriptEngine();
		if (!ctx.engine)
		{
			return;
		}

		ctx.engine->SetContextScene(m_Scene);

		if (ctx.scriptEngineType)
		{
			if (ManagedCallbacks_::OnRenderUI)
			{
				ManagedCallbacks_::OnRenderUI();
			}
		}

		ctx.engine->SetContextScene(nullptr);
	}

} // namespace Chained

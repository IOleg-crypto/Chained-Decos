#include "scene_scripting_manager.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"
#include "engine/physics/physics.h"
#include "engine/scene/scene.h"
#include "scripting/scriptengine.h"
#include "scripting/scriptengine_services.h"
#include "scripting/script_interop_pointers.h"
#include <Coral/String.hpp>
#include <Coral/Type.hpp>
#include <memory>

namespace Chained
{

namespace
{
// Dummy deleter for instances since we no longer track ManagedObject wrapper natively
void DummyDeleter(void* ptr)
{
    delete static_cast<int*>(ptr);
}
} // namespace

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
    std::lock_guard<std::mutex> lock(s_ManagersMutex);
    for (auto* manager : s_Managers)
    {
        manager->OnRuntimeStop();
    }
}

SceneScriptingManager::SceneScriptingManager(Scene* scene)
    : m_Scene(scene)
{
    SceneScriptingManager::Register(this);
}

SceneScriptingManager::~SceneScriptingManager()
{
    SceneScriptingManager::Unregister(this);

    if (m_Scene)
    {
        ServiceLocator::Get<Physics>()->SetCollisionCallback(m_Scene, nullptr);

        if (m_Scene->IsSimulationRunning())
        {
            auto engine = ServiceLocator::Get<ScriptEngine>();
            if (engine && engine->GetHost().IsInitialized())
            {
                auto* coreAssembly = engine->GetHost().GetCoreAssembly();
                if (coreAssembly)
                {
                    Coral::Type scriptEngineType = coreAssembly->GetLocalType("Chained.ScriptEngine");
                    if (scriptEngineType)
                    {
                        if (g_ManagedPointers.ScriptEngine_ClearAll)
                            g_ManagedPointers.ScriptEngine_ClearAll();
                    }
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
            script.Instance.reset();
            script.NeedsStart = true;
        }
    }

    entt::registry* registryPtr = m_Scene->GetRegistryPtr();
    ServiceLocator::Get<Physics>()->SetCollisionCallback(m_Scene, [registryPtr](entt::entity a, entt::entity b) {
        if (!registryPtr)
        {
            return;
        }

        auto engine = ServiceLocator::Get<ScriptEngine>();
        if (!engine || !engine->GetHost().IsInitialized())
        {
            return;
        }

        auto* coreAssembly = engine->GetHost().GetCoreAssembly();
        if (!coreAssembly)
        {
            return;
        }

        Coral::Type scriptEngineType = coreAssembly->GetLocalType("Chained.ScriptEngine");
        if (scriptEngineType)
        {
            if (g_ManagedPointers.ScriptEngine_OnCollisionEnter)
                g_ManagedPointers.ScriptEngine_OnCollisionEnter((uint64_t)(uint32_t)a, (uint64_t)(uint32_t)b);
        }
    });
}

void SceneScriptingManager::OnRuntimeStop()
{
    ServiceLocator::Get<Physics>()->SetCollisionCallback(m_Scene, nullptr);

    auto engine = ServiceLocator::Get<ScriptEngine>();
    if (engine && engine->GetHost().IsInitialized() && !m_ReloadInProgress)
    {
        auto* coreAssembly = engine->GetHost().GetCoreAssembly();
        if (coreAssembly)
        {
            Coral::Type scriptEngineType = coreAssembly->GetLocalType("Chained.ScriptEngine");
            if (scriptEngineType)
            {
                if (g_ManagedPointers.ScriptEngine_ClearAll)
                    g_ManagedPointers.ScriptEngine_ClearAll();
            }
        }
    }

    auto& registry = m_Scene->GetRegistry();
    auto view = registry.view<ManagedScriptComponent>();
    for (auto entity : view)
    {
        auto& msc = registry.get<Chained::ManagedScriptComponent>(entity);
        for (auto& script : msc.Scripts)
        {
            script.Instance.reset();
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

    auto engine = ServiceLocator::Get<ScriptEngine>();
    if (!engine->GetHost().IsInitialized() || m_ReloadInProgress)
    {
        return;
    }

    SetContextScene(m_Scene);

    auto* coreAssembly = engine->GetHost().GetCoreAssembly();
    if (!coreAssembly)
    {
        SetContextScene(nullptr);
        return;
    }

    Coral::Type scriptEngineType = coreAssembly->GetLocalType("Chained.ScriptEngine");
    if (!scriptEngineType)
    {
        SetContextScene(nullptr);
        return;
    }

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
                    Coral::String classNameStr = Coral::String::New(script.ClassName);
                    if (g_ManagedPointers.ScriptEngine_InstantiateScript)
                        g_ManagedPointers.ScriptEngine_InstantiateScript((uint64_t)(uint32_t)entity, (const char16_t*)classNameStr.Data());
                    Coral::String::Free(classNameStr);
                    CH_CORE_INFO("C++ calling InstantiateScript SUCCESS");

                    for (const auto& [fieldName, field] : script.Fields)
                    {
                        CH_CORE_INFO("C++ setting field {}", fieldName);
                        Coral::String fNameStr = Coral::String::New(fieldName);
                        Coral::String cNameStr = Coral::String::New(script.ClassName);
                        if (field.Type == ScriptFieldType::Float)
                        {
                            scriptEngineType.InvokeStaticMethod("SetFieldFloat", (uint64_t)(uint32_t)entity, cNameStr,
                                                                fNameStr, std::get<float>(field.Value));
                        }
                        else if (field.Type == ScriptFieldType::Int)
                        {
                            scriptEngineType.InvokeStaticMethod("SetFieldInt", (uint64_t)(uint32_t)entity, cNameStr,
                                                                fNameStr, std::get<int>(field.Value));
                        }
                        else if (field.Type == ScriptFieldType::Bool)
                        {
                            scriptEngineType.InvokeStaticMethod("SetFieldBool", (uint64_t)(uint32_t)entity, cNameStr,
                                                                fNameStr, std::get<bool>(field.Value));
                        }
                        else if (field.Type == ScriptFieldType::String)
                        {
                            Coral::String vStr = Coral::String::New(std::get<std::string>(field.Value));
                            scriptEngineType.InvokeStaticMethod("SetFieldString", (uint64_t)(uint32_t)entity, cNameStr,
                                                                fNameStr, vStr);
                            Coral::String::Free(vStr);
                        }
                        Coral::String::Free(cNameStr);
                        Coral::String::Free(fNameStr);
                    }

                    // Mark instantiated on C++ side
                    script.Instance = std::shared_ptr<void>(new int(1), DummyDeleter);
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
        if (g_ManagedPointers.ScriptEngine_OnUpdate)
            g_ManagedPointers.ScriptEngine_OnUpdate((float)deltaTime);
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

    auto engine = ServiceLocator::Get<ScriptEngine>();
    if (!engine->GetHost().IsInitialized())
    {
        return;
    }

    auto* coreAssembly = engine->GetHost().GetCoreAssembly();
    if (!coreAssembly)
    {
        return;
    }

    SetContextScene(m_Scene);

    Coral::Type scriptEngineType = coreAssembly->GetLocalType("Chained.ScriptEngine");
    if (scriptEngineType)
    {
        if (g_ManagedPointers.ScriptEngine_OnEvent)
            g_ManagedPointers.ScriptEngine_OnEvent((int)e.GetEventType());
    }

    SetContextScene(nullptr);
}

void SceneScriptingManager::OnRenderUI()
{
    if (m_ReloadInProgress)
    {
        return;
    }

    auto engine = ServiceLocator::Get<ScriptEngine>();
    if (!engine || !engine->GetHost().IsInitialized())
    {
        return;
    }

    auto* coreAssembly = engine->GetHost().GetCoreAssembly();
    if (!coreAssembly)
    {
        return;
    }

    SetContextScene(m_Scene);

    Coral::Type scriptEngineType = coreAssembly->GetLocalType("Chained.ScriptEngine");
    if (scriptEngineType)
    {
        if (g_ManagedPointers.ScriptEngine_OnRenderUI)
            g_ManagedPointers.ScriptEngine_OnRenderUI();
    }

    SetContextScene(nullptr);
}

} // namespace Chained

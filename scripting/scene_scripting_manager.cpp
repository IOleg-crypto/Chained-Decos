#include "scene_scripting_manager.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"
#include "engine/physics/physics.h"
#include "engine/scene/scene.h"
#include "scripting/scriptengine.h"
#include "scripting/scriptengine_services.h"
#include <Coral/ManagedObject.hpp>
#include <memory>

namespace Chained
{

namespace
{
// Custom deleter that properly shuts down a Coral::ManagedObject before freeing it.
// Used as the deleter for shared_ptr<void> so Coral headers are not needed in scripting_components.h.
void CoralObjectDeleter(void* ptr)
{
    delete static_cast<Coral::ManagedObject*>(ptr);
}
} // namespace

// Helper to cast the type-erased shared_ptr back to the concrete Coral type.
static inline Coral::ManagedObject* AsManagedObject(const ManagedScriptInstance& script)
{
    return static_cast<Coral::ManagedObject*>(script.GetRaw());
}

static void DestroyManagedScriptInstance(ManagedScriptInstance& script)
{
    if (script.HasInstance())
    {
        auto* obj = AsManagedObject(script);
        if (obj->IsValid())
        {
            obj->InvokeMethod("OnDestroy");
        }
    }

    script.Instance.reset();
    script.NeedsStart = true;
}

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
        manager->OnRuntimeStop(); // This will Destroy() all instances
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
        
        Physics::SetCollisionCallback(m_Scene, nullptr);

        if (m_Scene->IsSimulationRunning())
        {
            auto& registry = m_Scene->GetRegistry();
            auto view = registry.view<ManagedScriptComponent>();
            for (auto entity : view)
            {
                auto& msc = registry.get<Chained::ManagedScriptComponent>(entity);
                for (auto& script : msc.Scripts)
                {
                    if (script.HasInstance())
                    {
                        auto* obj = AsManagedObject(script);
                        if (obj && obj->IsValid())
                        {
                            obj->InvokeMethod("OnDestroy");
                        }
                    }
                }
            }
        }
    }
}

void SceneScriptingManager::OnRuntimeStart()
{
    CH_CORE_INFO("SceneScriptingManager::OnRuntimeStart - Entry");
    if (!m_Scene)
    {
        CH_CORE_ERROR("m_Scene is NULL!");
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
    CH_CORE_INFO("SceneScriptingManager::OnRuntimeStart - Setting collision callback");
    Physics::SetCollisionCallback(m_Scene, [registryPtr](entt::entity a, entt::entity b) {
        if (!registryPtr)
        {
            return;
        }

        auto& registry = *registryPtr;
        if (!registry.valid(a) || !registry.valid(b))
        {
            return;
        }

        // Dispatch to object A
        if (registry.all_of<ManagedScriptComponent>(a))
        {
            auto& msc = registry.get<ManagedScriptComponent>(a);
            for (auto& script : msc.Scripts)
            {
                if (script.HasInstance())
                {
                    auto* obj = AsManagedObject(script);
                    if (obj->IsValid())
                    {
                        obj->InvokeMethod("OnCollisionEnter", (uint64_t)(uint32_t)b);
                    }
                }
            }
        }

        // Dispatch to object B
        if (registry.all_of<ManagedScriptComponent>(b))
        {
            auto& msc = registry.get<ManagedScriptComponent>(b);
            for (auto& script : msc.Scripts)
            {
                if (script.HasInstance())
                {
                    auto* obj = AsManagedObject(script);
                    if (obj->IsValid())
                    {
                        obj->InvokeMethod("OnCollisionEnter", (uint64_t)(uint32_t)a);
                    }
                }
            }
        }
    });
}

void SceneScriptingManager::OnRuntimeStop()
{
    Physics::SetCollisionCallback(m_Scene, nullptr);

    auto& registry = m_Scene->GetRegistry();
    auto view = registry.view<ManagedScriptComponent>();
    for (auto entity : view)
    {
        auto& msc = registry.get<Chained::ManagedScriptComponent>(entity);
        for (auto& script : msc.Scripts)
        {
            DestroyManagedScriptInstance(script);
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

    if (!ServiceLocator::Get<ScriptEngine>()->GetHost().IsInitialized() || m_ReloadInProgress)
    {
        return;
    }

    
    SetContextScene(m_Scene);

    auto& registry = m_Scene->GetRegistry();

    
    std::vector<entt::entity> entities;
    auto view = registry.view<ManagedScriptComponent>();
    for (auto entity : view)
    {
        entities.push_back(entity);
    }

    for (auto entity : entities)
    {
        if (!registry.valid(entity) || !registry.all_of<ManagedScriptComponent>(entity))
        {
            continue;
        }

        size_t scriptCount = registry.get<ManagedScriptComponent>(entity).Scripts.size();
        for (size_t i = 0; i < scriptCount; ++i)
        {
            auto& msc = registry.get<ManagedScriptComponent>(entity);
            auto& script = msc.Scripts[i];

            
            if (!script.HasInstance() && !script.ClassName.empty())
            {
                auto* type = ServiceLocator::Get<ScriptEngine>()->GetRegistry().GetScriptClass(script.ClassName);
                if (type)
                {
                    try
                    {
                        CH_CORE_INFO("ScriptEngine: Instantiating '{}' for Runtime Entity ID: {}", script.ClassName,
                                     (uint32_t)entity);

                        auto* obj = new Coral::ManagedObject(type->CreateInstance());
                        auto sharedObj = std::shared_ptr<void>(obj, CoralObjectDeleter);

                        ManagedScriptInstance stableInstance = script;
                        stableInstance.Instance = sharedObj;

                        
                        obj->InvokeMethod("__Init", (uint64_t)(uint32_t)entity);

                        
                        auto& finalScript = registry.get<ManagedScriptComponent>(entity).Scripts[i];

                        finalScript.Instance = std::move(stableInstance.Instance);
                        finalScript.NeedsStart = true;

                        auto* finalObj = AsManagedObject(finalScript);
                        if (!finalObj)
                        {
                            continue;
                        }

                        
                        for (const auto& [fieldName, field] : finalScript.Fields)
                        {
                            try
                            {
                                std::visit([&](auto&& val) { finalObj->SetFieldValue(fieldName, val); }, field.Value);
                            } catch (const std::exception& e)
                            {
                                CH_CORE_WARN("ScriptEngine: Field sync skip '{}': {}", fieldName, e.what());
                            }
                        }

                        finalObj->InvokeMethod("OnCreate");
                    } catch (const std::exception& e)
                    {
                        CH_CORE_ERROR("ScriptEngine: Exception instantiating '{}': {}", script.ClassName, e.what());
                        registry.get<ManagedScriptComponent>(entity).Scripts[i].Instance.reset();
                    }
                }
            }

            
            auto& scriptLifecycle = registry.get<ManagedScriptComponent>(entity).Scripts[i];
            if (scriptLifecycle.HasInstance())
            {
                auto* obj = AsManagedObject(scriptLifecycle);
                if (!obj->IsValid())
                {
                    continue;
                }

                try
                {
                    if (scriptLifecycle.NeedsStart)
                    {
                        obj->InvokeMethod("OnStart");
                        scriptLifecycle.NeedsStart = false;
                    }

                    obj->InvokeMethod("OnUpdate", (float)deltaTime);
                } catch (const std::exception& e)
                {
                    CH_CORE_ERROR("ScriptEngine: Exception in OnUpdate for '{}': {}", scriptLifecycle.ClassName,
                                  e.what());
                }
            }
        }
    }

    SetContextScene(nullptr);
}

void SceneScriptingManager::OnEvent(Event& e)
{
    if (m_ReloadInProgress || !m_Scene || e.GetEventType() == EventType::None)
    {
        return;
    }

    SetContextScene(m_Scene);
    auto& registry = m_Scene->GetRegistry();

    auto view = registry.view<ManagedScriptComponent>();
    for (auto entity : view)
    {
        auto& msc = registry.get<Chained::ManagedScriptComponent>(entity);
        for (auto& script : msc.Scripts)
        {
            if (script.HasInstance())
            {
                auto* obj = AsManagedObject(script);
                if (obj->IsValid())
                {

                    obj->InvokeMethod("OnEvent", (int)e.GetEventType());
                }
            }
        }
    }
    SetContextScene(nullptr);
}

void SceneScriptingManager::OnRenderUI()
{
    if (!ServiceLocator::Get<ScriptEngine>()->GetHost().IsInitialized() || m_ReloadInProgress)
    {
        return;
    }

    SetContextScene(m_Scene);

    auto& registry = m_Scene->GetRegistry();
    auto view = registry.view<ManagedScriptComponent>();

    for (auto entity : view)
    {
        auto& msc = registry.get<Chained::ManagedScriptComponent>(entity);
        for (auto& script : msc.Scripts)
        {
            if (script.HasInstance())
            {
                auto* obj = AsManagedObject(script);
                if (obj->IsValid())
                {
                    try
                    {
                        obj->InvokeMethod("OnGUI");
                    } catch (const std::exception& e)
                    {
                        CH_CORE_ERROR("ScriptEngine: Exception in OnGUI for '{}': {}", script.ClassName, e.what());
                    }
                }
            }
        }
    }

    SetContextScene(nullptr);
}

} // namespace Chained

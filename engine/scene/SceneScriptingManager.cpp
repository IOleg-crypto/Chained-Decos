#include "SceneScriptingManager.h"
#include "engine/physics/physics.h"
#include "engine/core/profiler.h"
#include "scripting/script_glue.h"
#include "scripting/scriptengine.h"
#include "scripting/scriptengine_services.h"
#include <Coral/ManagedObject.hpp>
#include <memory>

namespace CHEngine
{

namespace
{
// RAII scope that registers the about-to-be-initialized script instance with ScriptGlue,
// then clears it on exit, so that __Init callbacks have the correct pending context.
class PendingScriptInstanceScope
{
public:
    explicit PendingScriptInstanceScope(ManagedScriptInstance* instance)
    {
        ScriptGlue::SetPendingScriptInstance(instance);
    }

    ~PendingScriptInstanceScope()
    {
        ScriptGlue::SetPendingScriptInstance(nullptr);
    }
};

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

static std::vector<SceneScriptingManager*> s_Managers;

void SceneScriptingManager::Register(SceneScriptingManager* manager)
{
    s_Managers.push_back(manager);
}

void SceneScriptingManager::Unregister(SceneScriptingManager* manager)
{
    auto it = std::find(s_Managers.begin(), s_Managers.end(), manager);
    if (it != s_Managers.end())
        s_Managers.erase(it);
}

void SceneScriptingManager::ResetAll()
{
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
    
    // shared_ptr<void> with CoralObjectDeleter handles cleanup automatically via RAII.
    // We still iterate to invoke OnDestroy callbacks before the object is deleted.
    if (m_Scene && m_Scene->IsSimulationRunning())
    {
        auto view = m_Scene->GetRegistry().view<ManagedScriptComponent>();
        for (auto entity : view)
        {
            auto& msc = view.get<ManagedScriptComponent>(entity);
            for (auto& script : msc.Scripts)
            {
                if (script.HasInstance())
                {
                    auto* obj = AsManagedObject(script);
                    if (obj->IsValid() && script.OnDestroy)
                        script.OnDestroy();
                }
            }
        }
    }
    // shared_ptr goes out of scope automatically — no manual delete needed.
}

void SceneScriptingManager::OnRuntimeStart()
{
    std::weak_ptr<entt::registry> weakRegistry = m_Scene->GetRegistryPtr();
    Physics::SetCollisionCallback(m_Scene, [weakRegistry](entt::entity a, entt::entity b) {
        auto registryPtr = weakRegistry.lock();
        if (!registryPtr) return;

        auto& registry = *registryPtr;
        if (!registry.valid(a) || !registry.valid(b)) return;

        // Dispatch to object A
        if (registry.all_of<ManagedScriptComponent>(a))
        {
            auto& msc = registry.get<ManagedScriptComponent>(a);
            for (auto& script : msc.Scripts)
            {
                if (script.HasInstance() && script.OnCollisionEnter)
                    script.OnCollisionEnter((uint64_t)(uint32_t)b);
            }
        }

        // Dispatch to object B
        if (registry.all_of<ManagedScriptComponent>(b))
        {
            auto& msc = registry.get<ManagedScriptComponent>(b);
            for (auto& script : msc.Scripts)
            {
                if (script.HasInstance() && script.OnCollisionEnter)
                    script.OnCollisionEnter((uint64_t)(uint32_t)a);
            }
        }
    });
}

void SceneScriptingManager::OnRuntimeStop()
{
    Physics::SetCollisionCallback(m_Scene, nullptr);

    auto view = m_Scene->GetRegistry().view<ManagedScriptComponent>();
    for (auto entity : view)
    {
        auto& msc = view.get<ManagedScriptComponent>(entity);
        for (auto& script : msc.Scripts)
        {
            script.Destroy();
        }
    }
}

void SceneScriptingManager::OnUpdate(Timestep deltaTime)
{
    CH_PROFILE_FUNCTION();

    if (!GetScriptHost().IsInitialized() || m_ReloadInProgress)
    {
        return;
    }

    SetContextScene(m_Scene);

    auto& registry = m_Scene->GetRegistry();
    auto view = registry.view<ManagedScriptComponent>();

    for (auto&& [entity, msc] : view.each())
    {
        for (auto& script : msc.Scripts)
        {
            // 1. Instantiation Phase
            if (!script.HasInstance() && !script.ClassName.empty())
            {
                auto* type = GetScriptRegistry().GetScriptClass(script.ClassName);
                if (type)
                {
                    try
                    {
                        // Allocate via raw pointer, wrap in shared_ptr with type-erasing deleter.
                        auto* obj = new Coral::ManagedObject(type->CreateInstance());
                        script.Instance = std::shared_ptr<void>(obj, CoralObjectDeleter);

                        PendingScriptInstanceScope pendingScope(&script);
                        obj->InvokeMethod("__Init", (uint64_t)(uint32_t)entity);

                        for (const auto& [fieldName, field] : script.Fields)
                        {
                            std::visit([&](auto&& val) { obj->SetFieldValue(fieldName, val); }, field.Value);
                        }

                        if (script.OnCreate) script.OnCreate();
                        else obj->InvokeMethod("OnCreate");

                        script.NeedsStart = true;
                    } catch (const std::exception& e)
                    {
                        CH_CORE_ERROR("ScriptEngine: Exception instantiating script '{}': {}", script.ClassName, e.what());
                        script.Instance.reset(); // Releases via CoralObjectDeleter automatically
                    }
                }
            }

            // 2. Lifecycle Execution Phase
            if (script.HasInstance())
            {
                auto* obj = AsManagedObject(script);
                if (!obj->IsValid()) continue;

                try
                {
                    if (script.NeedsStart)
                    {
                        if (script.OnStart) script.OnStart();
                        else obj->InvokeMethod("OnStart");
                        script.NeedsStart = false;
                    }

                    if (script.OnUpdate) script.OnUpdate((float)deltaTime);
                    else obj->InvokeMethod("OnUpdate", (float)deltaTime);
                } catch (const std::exception& e)
                {
                    CH_CORE_ERROR("ScriptEngine: Exception in script lifecycle for '{}': {}", script.ClassName, e.what());
                }
            }
        }
    }

    SetContextScene(nullptr);
}

void SceneScriptingManager::OnEvent(Event& e)
{
    if (m_ReloadInProgress || !m_Scene || e.GetEventType() == EventType::None)
        return;

    SetContextScene(m_Scene);
    auto& registry = m_Scene->GetRegistry();

    auto view = registry.view<ManagedScriptComponent>();
    for (auto&& [entity, msc] : view.each())
    {
        for (auto& script : msc.Scripts)
        {
            if (script.HasInstance())
            {
                auto* obj = AsManagedObject(script);
                if (obj->IsValid())
                {
                    try
                    {
                        if (script.OnEvent) script.OnEvent((int)e.GetEventType());
                    } catch (...)
                    {
                        // Scripts may not implement OnEvent — that's fine.
                    }
                }
            }
        }
    }
    SetContextScene(nullptr);
}

void SceneScriptingManager::OnRenderUI()
{
    if (!GetScriptHost().IsInitialized() || m_ReloadInProgress) return;

    SetContextScene(m_Scene);

    auto& registry = m_Scene->GetRegistry();
    auto view = registry.view<ManagedScriptComponent>();

    for (auto&& [entity, msc] : view.each())
    {
        for (auto& script : msc.Scripts)
        {
            if (script.HasInstance() && script.OnGUI)
            {
                auto* obj = AsManagedObject(script);
                if (obj->IsValid())
                {
                    try
                    {
                        script.OnGUI();
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

void ManagedScriptInstance::Destroy()
{
    if (HasInstance())
    {
        auto* obj = static_cast<Coral::ManagedObject*>(GetRaw());
        if (OnDestroy) OnDestroy();
        else if (obj->IsValid()) obj->InvokeMethod("OnDestroy");
    }
    ResetRuntimeState();
}

void ManagedScriptInstance::ResetRuntimeState()
{
    Instance.reset(); // shared_ptr deleter handles Coral::ManagedObject cleanup
    NeedsStart = true;
    OnCreate = nullptr;
    OnStart = nullptr;
    OnUpdate = nullptr;
    OnDestroy = nullptr;
    OnGUI = nullptr;
    OnCollisionEnter = nullptr;
    OnEvent = nullptr;
}

} // namespace CHEngine

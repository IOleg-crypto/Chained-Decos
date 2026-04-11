#include "scene_scripting.h"
#include "engine/physics/physics.h"
#include "engine/core/profiler.h"
#include "engine/scene/components.h"
#include "script_glue.h"
#include "scriptengine.h"
#include <Coral/ManagedObject.hpp>
#include <memory>

namespace CHEngine
{

namespace
{
class ActiveSceneScope
{
public:
    ActiveSceneScope(ScriptEngine& engine, Scene* scene)
        : m_Engine(engine)
    {
        m_Engine.SetActiveScene(scene);
    }

    ~ActiveSceneScope()
    {
        m_Engine.SetActiveScene(nullptr);
    }

private:
    ScriptEngine& m_Engine;
};

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
} // namespace

void SceneScripting::OnRuntimeStart(Scene* scene)
{
    std::weak_ptr<entt::registry> weakRegistry = scene->GetRegistryPtr();
    Physics::SetCollisionCallback(scene, [weakRegistry](entt::entity a, entt::entity b) {
        auto registryPtr = weakRegistry.lock();
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
                if (script.Instance && script.OnCollisionEnter)
                {
                    script.OnCollisionEnter((uint64_t)(uint32_t)b);
                }
            }
        }

        // Dispatch to object B
        if (registry.all_of<ManagedScriptComponent>(b))
        {
            auto& msc = registry.get<ManagedScriptComponent>(b);
            for (auto& script : msc.Scripts)
            {
                if (script.Instance && script.OnCollisionEnter)
                {
                    script.OnCollisionEnter((uint64_t)(uint32_t)a);
                }
            }
        }
    });
}

void SceneScripting::OnRuntimeStop(Scene* scene)
{
    Physics::SetCollisionCallback(scene, nullptr);
}

void SceneScripting::Update(Scene* scene, Timestep deltaTime)
{
    CH_PROFILE_FUNCTION();

    auto& scriptEngine = ScriptEngine::Get();

    if (!scriptEngine.CanExecuteFrameScripts())
    {
        static bool s_WarnedOnce = false;
        if (!s_WarnedOnce && !scriptEngine.IsInitialized())
        {
            CH_CORE_WARN("SceneScripting::Update - ScriptEngine not initialized. Scripts will not be updated.");
            s_WarnedOnce = true;
        }
        return;
    }

    ActiveSceneScope activeScene(scriptEngine, scene);

    auto& registry = scene->GetRegistry();
    auto view = registry.view<ManagedScriptComponent>();

    // Debug log to trace total entities with ManagedScriptComponent
    static bool s_LoggedOnce = false;
    if (!s_LoggedOnce)
    {
        int count = 0;
        for (auto e : view)
        {
            count++;
        }
        CH_CORE_INFO("SceneScripting::Update - Found {} entities with ManagedScriptComponent", count);
        s_LoggedOnce = true;
    }

    for (auto&& [entity, msc] : view.each())
    {
        for (auto& script : msc.Scripts)
        {
            // 1. Instantiation Phase
            if (!script.Instance && !script.ClassName.empty())
            {
                auto* type = scriptEngine.GetScriptClass(script.ClassName);
                if (type)
                {
                    CH_CORE_INFO("SceneScripting::Update - Instantiating script '{}' for entity '{}'", script.ClassName,
                                 (uint32_t)entity);
                    Coral::ManagedObject* obj = nullptr;
                    try
                    {
                        // First, create the instance (calls C# constructor)
                        obj = new Coral::ManagedObject(type->CreateInstance());

                        // Immediately inject the entity ID before OnCreate
                        // Note: SetPendingScriptInstance allows the script to register its delegates
                        PendingScriptInstanceScope pendingScriptInstance(&script);
                        obj->InvokeMethod("__Init", (uint64_t)(uint32_t)entity);

                        // 2. Apply persistent field values
                        for (const auto& [fieldName, field] : script.Fields)
                        {
                            std::visit([&](auto&& val) { obj->SetFieldValue(fieldName, val); }, field.Value);
                        }

                        // Initialize logic
                        if (script.OnCreate)
                        {
                            script.OnCreate();
                        }
                        else
                        {
                            obj->InvokeMethod("OnCreate");
                        }

                        script.Instance = obj;
                        script.NeedsStart = true;
                    } catch (const std::exception& e)
                    {
                        CH_CORE_ERROR("ScriptEngine: Exception instantiating script '{}': {}", script.ClassName,
                                      e.what());
                        if (obj)
                        {
                            delete obj;
                        }
                    } catch (...)
                    {
                        CH_CORE_ERROR("ScriptEngine: Unknown exception instantiating script '{}'", script.ClassName);
                        if (obj)
                        {
                            delete obj;
                        }
                    }
                }
            }

            // 2. Lifecycle Execution Phase
            if (script.Instance)
            {
                auto* obj = static_cast<Coral::ManagedObject*>(script.Instance);
                if (!obj->IsValid())
                {
                    continue;
                }

                try
                {
                    // OnStart: called once on the first frame after creation
                    if (script.NeedsStart)
                    {
                        if (script.OnStart)
                        {
                            script.OnStart();
                        }
                        else
                        {
                            obj->InvokeMethod("OnStart");
                        }

                        script.NeedsStart = false;
                    }

                    // OnUpdate: called every frame
                    if (script.OnUpdate)
                    {
                        script.OnUpdate((float)deltaTime);
                    }
                    else
                    {
                        obj->InvokeMethod("OnUpdate", (float)deltaTime);
                    }
                } catch (const std::exception& e)
                {
                    CH_CORE_ERROR("ScriptEngine: Exception in script lifecycle for '{}': {}", script.ClassName,
                                  e.what());
                }
            }
        }
    }
}

void SceneScripting::Stop(Scene* scene)
{
    auto& scriptEngine = ScriptEngine::Get();

    if (!scriptEngine.IsInitialized())
    {
        return;
    }

    ActiveSceneScope activeScene(scriptEngine, scene);

    scene->GetRegistry().view<ManagedScriptComponent>().each([&](auto entity, auto& msc) {
        for (auto& script : msc.Scripts)
        {
            if (script.Instance)
            {
                auto* obj = static_cast<Coral::ManagedObject*>(script.Instance);
                try
                {
                    if (script.OnDestroy)
                    {
                        script.OnDestroy();
                    }
                    else
                    {
                        obj->InvokeMethod("OnDestroy");
                    }
                } catch (...)
                {
                }

                obj->Destroy();
                delete obj;
                script.Instance = nullptr;
                script.NeedsStart = false;

                // Clear delegates
                script.OnCreate = nullptr;
                script.OnStart = nullptr;
                script.OnUpdate = nullptr;
                script.OnDestroy = nullptr;
            }
        }
    });

}

void SceneScripting::DispatchEvent(Scene* scene, Event& e)
{
    // Future: dispatch specialized events to C# (e.g. OnCollisionEnter)
}

void SceneScripting::RenderUI(Scene* scene)
{
    auto& scriptEngine = ScriptEngine::Get();

    if (!scriptEngine.CanExecuteFrameScripts())
    {
        return;
    }

    ActiveSceneScope activeScene(scriptEngine, scene);

    auto& registry = scene->GetRegistry();
    auto view = registry.view<ManagedScriptComponent>();

    for (auto&& [entity, msc] : view.each())
    {
        for (auto& script : msc.Scripts)
        {
            if (script.Instance && script.OnGUI)
            {
                auto* obj = static_cast<Coral::ManagedObject*>(script.Instance);
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

}

} // namespace CHEngine

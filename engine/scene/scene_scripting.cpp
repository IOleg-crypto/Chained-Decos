#include "scene_scripting.h"
#include "scriptable_entity.h"
#include "engine/script/scriptengine.h"
#include "engine/script/script_glue.h"
#include "engine/scene/components.h"
#include <Coral/ManagedObject.hpp>

namespace CHEngine {

    void SceneScripting::Update(Scene* scene, Timestep deltaTime)
    {
        if (!ScriptEngine::IsInitialized()) {
            CH_CORE_WARN("SceneScripting::Update - ScriptEngine not initialized");
            return;
        }

        ScriptEngine::SetActiveScene(scene);
        
        auto& registry = scene->GetRegistry();
        auto view = registry.view<ManagedScriptComponent>();

        // Debug log to trace total entities with ManagedScriptComponent
        static bool s_LoggedOnce = false;
        if (!s_LoggedOnce) {
            int count = 0;
            for (auto e : view) count++;
            CH_CORE_INFO("SceneScripting::Update - Found {} entities with ManagedScriptComponent", count);
            s_LoggedOnce = true;
        }

        for (auto [entity, msc] : view.each())
        {
            for (auto& script : msc.Scripts)
            {
                // 1. Instantiation Phase
                if (!script.Instance && !script.ClassName.empty())
                {
                    auto* type = ScriptEngine::GetScriptClass(script.ClassName);
                    if (type) 
                    {
                        CH_CORE_INFO("SceneScripting::Update - Instantiating script '{}' for entity '{}'", script.ClassName, (uint32_t)entity);
                        Coral::ManagedObject* obj = nullptr;
                        try {
                            // First, create the instance (calls C# constructor)
                            obj = new Coral::ManagedObject(type->CreateInstance());
                            
                            // Immediately inject the entity ID before OnCreate
                            // Note: SetPendingScriptInstance allows the script to register its delegates
                            ScriptGlue::SetPendingScriptInstance(&script);
                            obj->InvokeMethod("__Init", (uint64_t)(uint32_t)entity);
                            ScriptGlue::SetPendingScriptInstance(nullptr);
                            
                            // Initialize logic
                            if (script.OnCreate) script.OnCreate();
                            else obj->InvokeMethod("OnCreate");

                            
                            script.Instance = obj;
                            script.NeedsStart = true;
                        } 
                        catch (const std::exception& e) {
                            CH_CORE_ERROR("ScriptEngine: Exception instantiating script '{}': {}", script.ClassName, e.what());
                            if (obj) { delete obj; }
                        }
                        catch (...) {
                            CH_CORE_ERROR("ScriptEngine: Unknown exception instantiating script '{}'", script.ClassName);
                            if (obj) { delete obj; }
                        }
                    }
                }

                // 2. Lifecycle Execution Phase
                if (script.Instance) 
                {
                    auto* obj = static_cast<Coral::ManagedObject*>(script.Instance);
                    if (!obj->IsValid()) continue;

                    try {
                        // OnStart: called once on the first frame after creation
                        if (script.NeedsStart) {
                            if (script.OnStart) script.OnStart();
                            else obj->InvokeMethod("OnStart");
                            
                            script.NeedsStart = false;
                        }

                        // OnUpdate: called every frame
                        if (script.OnUpdate) script.OnUpdate((float)deltaTime);
                        else obj->InvokeMethod("OnUpdate", (float)deltaTime);
                    } 
                    catch (const std::exception& e) {
                        CH_CORE_ERROR("ScriptEngine: Exception in script lifecycle for '{}': {}", script.ClassName, e.what());
                    }
                }
            }
        }
    }

    void SceneScripting::Stop(Scene* scene)
    {
        if (!ScriptEngine::IsInitialized())
            return;

        ScriptEngine::SetActiveScene(scene);

        scene->GetRegistry().view<ManagedScriptComponent>().each([&](auto entity, auto& msc) {
            for (auto& script : msc.Scripts)
            {
                if (script.Instance)
                {
                    auto* obj = static_cast<Coral::ManagedObject*>(script.Instance);
                    try {
                        if (script.OnDestroy) script.OnDestroy();
                        else obj->InvokeMethod("OnDestroy");
                    } catch (...) {}

                    obj->Destroy();
                    delete obj;
                    script.Instance = nullptr;
                    script.NeedsStart = false;
                    
                    // Clear delegates
                    script.OnCreate = nullptr;
                    script.OnStart  = nullptr;
                    script.OnUpdate = nullptr;
                    script.OnDestroy = nullptr;
                }
            }
        });

        ScriptEngine::SetActiveScene(nullptr);
    }

    void SceneScripting::DispatchEvent(Scene* scene, Event& e)
    {
        // Future: dispatch specialized events to C# (e.g. OnCollisionEnter)
    }

    void SceneScripting::RenderUI(Scene* scene)
    {
        // Future: dispatch OnGUI to C# scripts if implemented
    }

} // namespace CHEngine

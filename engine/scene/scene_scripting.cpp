#include "scene_scripting.h"
#include "scriptable_entity.h"

namespace CHEngine
{
void SceneScripting::Update(Scene *scene, Timestep deltaTime)
{
    int scriptableEntities = 0;
    int scriptsUpdated = 0;
    
    scene->GetRegistry().view<NativeScriptComponent>().each(
        [&](auto entity, auto &nsc)
        {
            scriptableEntities++;
            std::string tagName = scene->GetRegistry().get<TagComponent>(entity).Tag;
            
            for (auto &script : nsc.Scripts)
            {
                if (!script.Instance && script.InstantiateScript)
                {
                    script.Instance = script.InstantiateScript();
                    script.Instance->m_Entity = Entity(entity, &scene->GetRegistry());
                    script.Instance->m_Scene = scene;
                    CH_CORE_INFO("[SCRIPT_DIAG] NativeScript: {} - Lazy-instantiating script '{}'", tagName, script.ScriptName);
                    script.Instance->OnCreate();
                }
                
                if (script.Instance)
                {
                    script.Instance->OnUpdate(deltaTime);
                    scriptsUpdated++;
                }
                else
                {
                    CH_CORE_WARN("[SCRIPT_DIAG] NativeScript: {} - Script '{}' has NO instance!", tagName, script.ScriptName);
                }
            }
        });
}

void SceneScripting::DispatchEvent(Scene *scene, Event &e)
{
    scene->GetRegistry().view<NativeScriptComponent>().each(
        [&](auto entity, auto &nsc)
        {
            for (auto &script : nsc.Scripts)
            {
                if (script.Instance)
                {
                    script.Instance->OnEvent(e);
                }
            }
        });
}

void SceneScripting::RenderUI(Scene *scene)
{
    scene->GetRegistry().view<NativeScriptComponent>().each(
        [&](auto entity, auto &nsc)
        {
            for (auto &script : nsc.Scripts)
            {
                if (script.Instance)
                {
                    script.Instance->OnImGuiRender();
                }
            }
        });
}
} // namespace CHEngine

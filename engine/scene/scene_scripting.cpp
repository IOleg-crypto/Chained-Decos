#include "scene_scripting.h"
#include "scriptable_entity.h"

namespace CHEngine
{
void SceneScripting::Update(Scene *scene, float deltaTime)
{
    scene->GetRegistry().view<NativeScriptComponent>().each(
        [&](auto entity, auto &nsc)
        {
            for (auto &script : nsc.Scripts)
            {
                if (!script.Instance)
                {
                    script.Instance = script.InstantiateScript();
                    script.Instance->m_Entity = Entity{entity, scene};
                    script.Instance->OnCreate();
                }
                script.Instance->OnUpdate(deltaTime);
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

#include "scene_scripting.h"
#include "components.h"
#include "engine/core/profiler.h"
#include "scene.h"
#include "scriptable_entity.h"

namespace CHEngine
{
void SceneScripting::OnUpdate(float deltaTime)
{
    CH_PROFILE_FUNCTION();
    auto &registry = m_Scene->GetRegistry();

    registry.view<NativeScriptComponent>().each(
        [this, deltaTime](auto entity, auto &nsc)
        {
            for (auto &script : nsc.Scripts)
            {
                if (!script.Instance)
                {
                    script.Instance = script.InstantiateScript();
                    script.Instance->m_Entity = Entity{entity, m_Scene};
                    CH_CORE_INFO("Scripting: Instantiating script '{0}' for entity '{1}'",
                                 script.ScriptName,
                                 m_Scene->GetRegistry().get<TagComponent>(entity).Tag);
                    script.Instance->OnCreate();
                }
                script.Instance->OnUpdate(deltaTime);
            }
        });
}

void SceneScripting::OnEvent(Event &e)
{
    auto &registry = m_Scene->GetRegistry();
    registry.view<NativeScriptComponent>().each(
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
} // namespace CHEngine

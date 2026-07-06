#include "script_glue_internal.h"
#include "engine/core/service_locator.h"
#include "scriptengine.h"

Chained::Scene* Chained::GetActiveScene()
{
    return ServiceLocator::Get<ScriptEngine>()->GetContextScene();
}

Chained::Entity Chained::GetEntity(uint64_t entityID)
{
    Scene* scene = GetActiveScene();
    if (!scene)
    {
        return {};
    }
    return Entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
}

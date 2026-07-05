#include "script_glue_internal.h"

Chained::Scene* Chained::GetActiveScene()
{
    return GetContextScene();
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

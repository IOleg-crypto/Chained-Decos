#include "engine/app/application.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/log.h"
#include "engine/scene/scene_events.h"
#include "script_glue_internal.h"
#include "script_internal_call_registry.h"


namespace Chained
{

CH_SCRIPT_FUNC uint64_t Scene_FindEntityByTag(Coral::String tag)
{
    auto* scene = GetActiveScene();
    if (scene)
    {
        auto entity = scene->FindEntityByTag((std::string)tag);
        return entity ? (uint64_t)(uint32_t)entity : 0;
    }
    return 0;
}

CH_SCRIPT_FUNC uint64_t Scene_CopyEntity(uint64_t entityID)
{
    auto* scene = GetActiveScene();
    if (scene)
    {
        auto entity = scene->CopyEntity((entt::entity)(uint32_t)entityID);
        return entity != entt::null ? (uint64_t)(uint32_t)entity : 0;
    }
    return 0;
}

CH_SCRIPT_FUNC void Scene_LoadScene(Coral::String path)
{
    SceneChangeRequestEvent e((std::string)path);
    Application::Get().OnEvent(e);
}

CH_SCRIPT_FUNC uint64_t Scene_GetPrimaryCameraEntity()
{
    auto* scene = GetActiveScene();
    if (!scene)
    {
        return 0;
    }

    auto& reg = scene->GetRegistry();
    auto view = reg.view<CameraComponent>();
    for (auto entity : view)
    {
        if (view.get<CameraComponent>(entity).Primary)
        {
            return (uint64_t)(uint32_t)entity;
        }
    }
    return 0;
}

void RegisterGlueScene(Coral::ManagedAssembly& assembly)
{
    assembly.AddInternalCall("Chained.Scene", "Scene_FindEntityByTag_Ptr", (void*)Scene_FindEntityByTag);
    assembly.AddInternalCall("Chained.Scene", "Scene_CopyEntity_Ptr", (void*)Scene_CopyEntity);
    assembly.AddInternalCall("Chained.Scene", "Scene_LoadScene_Ptr", (void*)Scene_LoadScene);
    assembly.AddInternalCall("Chained.Scene", "Scene_GetPrimaryCameraEntity_Ptr", (void*)Scene_GetPrimaryCameraEntity);
}
} // namespace Chained

#include "script_glue_internal.h"

namespace CHEngine
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
        return entity ? (uint64_t)(uint32_t)entity : 0;
    }
    return 0;
}

CH_SCRIPT_FUNC void Scene_LoadScene(Coral::String path)
{
    ScriptEngine::Get().RequestLoadScene((std::string)path);
}

CH_SCRIPT_FUNC uint64_t Scene_GetPrimaryCameraEntity()
{
    auto* scene = GetActiveScene();
    if (!scene)
    {
        return 0;
    }
    Entity entity = scene->GetPrimaryCameraEntity();
    return entity ? (uint64_t)(uint32_t)entity : 0;
}

void RegisterSceneGlue(Coral::ManagedAssembly& assembly) {
#define CH_ADD_INTERNAL_CALL(className, fieldName, funcPtr)                                                            \
    assembly.AddInternalCall("CHEngine." #className, #fieldName, (void*)funcPtr)

    CH_ADD_INTERNAL_CALL(Scene, Scene_FindEntityByTag_Ptr, Scene_FindEntityByTag);
    CH_ADD_INTERNAL_CALL(Scene, Scene_LoadScene_Ptr, Scene_LoadScene);
    CH_ADD_INTERNAL_CALL(Scene, Scene_GetPrimaryCameraEntity_Ptr, Scene_GetPrimaryCameraEntity);
    CH_ADD_INTERNAL_CALL(Scene, Scene_CopyEntity_Ptr, Scene_CopyEntity);

#undef CH_ADD_INTERNAL_CALL
} } // namespace CHEngine


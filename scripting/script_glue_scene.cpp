#include "script_glue_scene.h"
#include "engine/app/application.h"
#include "engine/core/log.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/components.h"

namespace Chained {
CH_SCRIPT_FUNC uint64_t Scene_FindEntityByTag(const Coral::UCChar* tag) {
    auto *scene = GetActiveScene();
    if (scene && tag) {
        auto entity = scene->FindEntityByTag(ch_u16_to_string(tag));
        return entity ? (uint64_t)(uint32_t)entity : 0;
    }
    return 0;
}

CH_SCRIPT_FUNC uint64_t Scene_CopyEntity(uint64_t entityID) {
    auto *scene = GetActiveScene();
    if (scene) {
        auto entity = scene->CopyEntity((entt::entity)(uint32_t)entityID);
        return entity != entt::null ? (uint64_t)(uint32_t)entity : 0;
    }
    return 0;
}

CH_SCRIPT_FUNC void Scene_LoadScene(const Coral::UCChar* path) {
    if (!path) return;
    SceneChangeRequestEvent e(ch_u16_to_string(path));
    Application::Get().OnEvent(e);
}

CH_SCRIPT_FUNC uint64_t Scene_GetPrimaryCameraEntity() {
    auto *scene = GetActiveScene();
    if (!scene) {
        return 0;
    }

    auto &reg = scene->GetRegistry();
    auto view = reg.view<CameraComponent>();
    for (auto entity : view) {
        if (view.get<CameraComponent>(entity).Primary) {
            return (uint64_t)(uint32_t)entity;
        }
    }
    return 0;
}

} // namespace Chained

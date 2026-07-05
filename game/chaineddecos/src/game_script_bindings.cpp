#include "components/game_components.h"
#include "scripting/script_glue_internal.h"
#include "scripting/script_internal_call_registry.h"
#include "scripting/script_glue_entity.h"
#include "engine/app/application.h"

namespace Chained {

    // ── Spawn / Transition ────────────────────────────────────────────────
    CH_SCRIPT_FUNC inline bool SpawnComponent_IsActive(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<SpawnComponent>() ? entity.GetComponent<SpawnComponent>().IsActive : false;
    }
    CH_ADD_INTERNAL_CALL(SpawnComponent, SpawnComponent_IsActive_Ptr, SpawnComponent_IsActive);

    CH_SCRIPT_FUNC inline Coral::String SceneTransitionComponent_GetTargetScene(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<SceneTransitionComponent>())
            return Coral::String::New(entity.GetComponent<SceneTransitionComponent>().TargetScenePath);
        return Coral::String::New("");
    }
    CH_ADD_INTERNAL_CALL(SceneTransitionComponent, SceneTransitionComponent_GetTargetScene_Ptr, SceneTransitionComponent_GetTargetScene);

    CH_SCRIPT_FUNC inline float PlayerComponent_GetJumpForce(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<PlayerComponent>() ? entity.GetComponent<PlayerComponent>().JumpForce : 0.0f;
    }
    CH_ADD_INTERNAL_CALL(PlayerComponent, PlayerComponent_GetJumpForce_Ptr, PlayerComponent_GetJumpForce);

    CH_SCRIPT_FUNC inline void PlayerComponent_SetJumpForce(uint64_t entityID, float force) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<PlayerComponent>())
            entity.GetComponent<PlayerComponent>().JumpForce = force;
    }
    CH_ADD_INTERNAL_CALL(PlayerComponent, PlayerComponent_SetJumpForce_Ptr, PlayerComponent_SetJumpForce);

    CH_SCRIPT_FUNC inline float PlayerComponent_GetLookSensitivity(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<PlayerComponent>() ? entity.GetComponent<PlayerComponent>().LookSensitivity : 0.0f;
    }
    CH_ADD_INTERNAL_CALL(PlayerComponent, PlayerComponent_GetLookSensitivity_Ptr, PlayerComponent_GetLookSensitivity);

    CH_SCRIPT_FUNC inline void PlayerComponent_SetLookSensitivity(uint64_t entityID, float sensitivity) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<PlayerComponent>())
            entity.GetComponent<PlayerComponent>().LookSensitivity = sensitivity;
    }
    CH_ADD_INTERNAL_CALL(PlayerComponent, PlayerComponent_SetLookSensitivity_Ptr, PlayerComponent_SetLookSensitivity);

} // namespace Chained

#include "components/game_components.h"
#include "scripting/script_glue_internal.h"
#include "scripting/script_internal_call_registry.h"
#include "engine/runtime/application.h"

namespace Chained {

    // ── Spawn / Transition ────────────────────────────────────────────────
    CH_SCRIPT_FUNC bool SpawnComponent_IsActive(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<SpawnComponent>() ? entity.GetComponent<SpawnComponent>().IsActive : false;
    }
    CH_ADD_INTERNAL_CALL(SpawnComponent, SpawnComponent_IsActive_Ptr, SpawnComponent_IsActive);

    CH_SCRIPT_FUNC void SpawnComponent_GetSpawnPoint(uint64_t entityID, glm::vec3* point) {
        Entity entity = GetEntity(entityID);
        if (!point) return;
        if (entity && entity.HasComponent<SpawnComponent>())
            *point = entity.GetComponent<SpawnComponent>().SpawnPoint;
        else
            *point = {0,0,0};
    }
    CH_ADD_INTERNAL_CALL(SpawnComponent, SpawnComponent_GetSpawnPoint_Ptr, SpawnComponent_GetSpawnPoint);

    CH_SCRIPT_FUNC Coral::String SceneTransitionComponent_GetTargetScene(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<SceneTransitionComponent>())
            return Coral::String::New(entity.GetComponent<SceneTransitionComponent>().TargetScenePath);
        return Coral::String::New("");
    }
    CH_ADD_INTERNAL_CALL(SceneTransitionComponent, SceneTransitionComponent_GetTargetScene_Ptr, SceneTransitionComponent_GetTargetScene);

    // ── RPGStatsComponent ────────────────────────────────────────────────
    CH_SCRIPT_FUNC int RPGStatsComponent_GetLevel(uint64_t entityID) {
        auto e = GetEntity(entityID);
        return e && e.HasComponent<RPGStatsComponent>() ? e.GetComponent<RPGStatsComponent>().Level : 0;
    }
    CH_ADD_INTERNAL_CALL(RPGStatsComponent, RPGStatsComponent_GetLevel_Ptr, RPGStatsComponent_GetLevel);

    CH_SCRIPT_FUNC void RPGStatsComponent_SetLevel(uint64_t entityID, int val) {
        auto e = GetEntity(entityID);
        if (e && e.HasComponent<RPGStatsComponent>()) e.GetComponent<RPGStatsComponent>().Level = val;
    }
    CH_ADD_INTERNAL_CALL(RPGStatsComponent, RPGStatsComponent_SetLevel_Ptr, RPGStatsComponent_SetLevel);

    CH_SCRIPT_FUNC float RPGStatsComponent_GetHealth(uint64_t entityID) {
        auto e = GetEntity(entityID);
        return e && e.HasComponent<RPGStatsComponent>() ? e.GetComponent<RPGStatsComponent>().Health : 0.0f;
    }
    CH_ADD_INTERNAL_CALL(RPGStatsComponent, RPGStatsComponent_GetHealth_Ptr, RPGStatsComponent_GetHealth);

    CH_SCRIPT_FUNC void RPGStatsComponent_SetHealth(uint64_t entityID, float val) {
        auto e = GetEntity(entityID);
        if (e && e.HasComponent<RPGStatsComponent>()) e.GetComponent<RPGStatsComponent>().Health = val;
    }
    CH_ADD_INTERNAL_CALL(RPGStatsComponent, RPGStatsComponent_SetHealth_Ptr, RPGStatsComponent_SetHealth);

    CH_SCRIPT_FUNC int RPGStatsComponent_GetGold(uint64_t entityID) {
        auto e = GetEntity(entityID);
        return e && e.HasComponent<RPGStatsComponent>() ? e.GetComponent<RPGStatsComponent>().Gold : 0;
    }
    CH_ADD_INTERNAL_CALL(RPGStatsComponent, RPGStatsComponent_GetGold_Ptr, RPGStatsComponent_GetGold);

    CH_SCRIPT_FUNC void RPGStatsComponent_SetGold(uint64_t entityID, int val) {
        auto e = GetEntity(entityID);
        if (e && e.HasComponent<RPGStatsComponent>()) e.GetComponent<RPGStatsComponent>().Gold = val;
    }
    CH_ADD_INTERNAL_CALL(RPGStatsComponent, RPGStatsComponent_SetGold_Ptr, RPGStatsComponent_SetGold);

    // ── SkillComponent ───────────────────────────────────────────────────
    CH_SCRIPT_FUNC bool SkillComponent_IsUnlocked(uint64_t entityID) {
        auto e = GetEntity(entityID);
        return e && e.HasComponent<SkillComponent>() ? e.GetComponent<SkillComponent>().IsUnlocked : false;
    }
    CH_ADD_INTERNAL_CALL(SkillComponent, SkillComponent_IsUnlocked_Ptr, SkillComponent_IsUnlocked);

    CH_SCRIPT_FUNC void SkillComponent_SetUnlocked(uint64_t entityID, bool val) {
        auto e = GetEntity(entityID);
        if (e && e.HasComponent<SkillComponent>()) e.GetComponent<SkillComponent>().IsUnlocked = val;
    }
    CH_ADD_INTERNAL_CALL(SkillComponent, SkillComponent_SetUnlocked_Ptr, SkillComponent_SetUnlocked);

    // ── PlayerComponent ───────────────────────────────────────────────────
    CH_SCRIPT_FUNC float PlayerComponent_GetMovementSpeed(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<PlayerComponent>() ? entity.GetComponent<PlayerComponent>().MovementSpeed : 0.0f;
    }
    CH_ADD_INTERNAL_CALL(PlayerComponent, PlayerComponent_GetMovementSpeed_Ptr, PlayerComponent_GetMovementSpeed);

    CH_SCRIPT_FUNC void PlayerComponent_SetMovementSpeed(uint64_t entityID, float speed) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<PlayerComponent>()) 
            entity.GetComponent<PlayerComponent>().MovementSpeed = speed;
    }
    CH_ADD_INTERNAL_CALL(PlayerComponent, PlayerComponent_SetMovementSpeed_Ptr, PlayerComponent_SetMovementSpeed);

    CH_SCRIPT_FUNC float PlayerComponent_GetJumpForce(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<PlayerComponent>() ? entity.GetComponent<PlayerComponent>().JumpForce : 0.0f;
    }
    CH_ADD_INTERNAL_CALL(PlayerComponent, PlayerComponent_GetJumpForce_Ptr, PlayerComponent_GetJumpForce);

    CH_SCRIPT_FUNC void PlayerComponent_SetJumpForce(uint64_t entityID, float force) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<PlayerComponent>())
            entity.GetComponent<PlayerComponent>().JumpForce = force;
    }
    CH_ADD_INTERNAL_CALL(PlayerComponent, PlayerComponent_SetJumpForce_Ptr, PlayerComponent_SetJumpForce);

    CH_SCRIPT_FUNC float PlayerComponent_GetLookSensitivity(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<PlayerComponent>() ? entity.GetComponent<PlayerComponent>().LookSensitivity : 0.0f;
    }
    CH_ADD_INTERNAL_CALL(PlayerComponent, PlayerComponent_GetLookSensitivity_Ptr, PlayerComponent_GetLookSensitivity);

    CH_SCRIPT_FUNC void PlayerComponent_SetLookSensitivity(uint64_t entityID, float sensitivity) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<PlayerComponent>())
            entity.GetComponent<PlayerComponent>().LookSensitivity = sensitivity;
    }
    CH_ADD_INTERNAL_CALL(PlayerComponent, PlayerComponent_SetLookSensitivity_Ptr, PlayerComponent_SetLookSensitivity);

    // ── NetworkIdentity ──────────────────────────────────────────────────
    CH_SCRIPT_FUNC uint64_t NetworkIdentity_GetNetworkID(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<NetworkIdentity>() ? entity.GetComponent<NetworkIdentity>().NetworkID : 0;
    }
    CH_ADD_INTERNAL_CALL(NetworkIdentity, NetworkIdentity_GetNetworkID_Ptr, NetworkIdentity_GetNetworkID);

    CH_SCRIPT_FUNC bool NetworkIdentity_IsOwned(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<NetworkIdentity>() ? entity.GetComponent<NetworkIdentity>().IsOwned : false;
    }
    CH_ADD_INTERNAL_CALL(NetworkIdentity, NetworkIdentity_IsOwned_Ptr, NetworkIdentity_IsOwned);

} // namespace Chained

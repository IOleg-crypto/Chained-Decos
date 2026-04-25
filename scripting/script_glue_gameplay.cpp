#include "script_glue_internal.h"
#include "script_internal_call_registry.h"

namespace CHEngine {

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

    // ── Audio ─────────────────────────────────────────────────────────────
    CH_SCRIPT_FUNC void Audio_Play(Coral::String path, float volume, float pitch, bool loop) {
        if (Project::GetActive() != nullptr) {
            const std::string soundPath = (std::string)path;
            AudioHandle handle = Audio::Get().LoadSound(soundPath);
            if (handle != 0) {
                Audio::Get().Play(handle, volume, pitch, loop);

                if (Scene* scene = GetActiveScene()) {
                    auto& registry = scene->GetRegistry();
                    auto view = registry.view<AudioComponent>();
                    for (auto entity : view) {
                        auto& audio = view.get<AudioComponent>(entity);
                        if (audio.SoundPath == soundPath) {
                            audio.SoundHandle = handle;
                            audio.IsPlaying = true;
                        }
                    }
                }
            }
        }
    }
    CH_ADD_INTERNAL_CALL(Audio, Audio_Play_Ptr, Audio_Play);

    CH_SCRIPT_FUNC void Audio_Stop(Coral::String path) {
        if (Project::GetActive() != nullptr) {
            const std::string soundPath = (std::string)path;
            Audio::Get().Stop(soundPath);

            if (Scene* scene = GetActiveScene()) {
                auto& registry = scene->GetRegistry();
                auto view = registry.view<AudioComponent>();
                for (auto entity : view) {
                    auto& audio = view.get<AudioComponent>(entity);
                    if (audio.SoundPath == soundPath) {
                        audio.IsPlaying = false;
                    }
                }
            }
        }
    }
    CH_ADD_INTERNAL_CALL(Audio, Audio_Stop_Ptr, Audio_Stop);

    CH_SCRIPT_FUNC void Audio_StopAll() {
        if (Project::GetActive() != nullptr) {
            Audio::Get().StopAll();
        }
    }
    CH_ADD_INTERNAL_CALL(Audio, Audio_StopAll_Ptr, Audio_StopAll);

    CH_SCRIPT_FUNC void AudioComponent_SetVolume(uint64_t entityID, float volume) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<AudioComponent>()) entity.GetComponent<AudioComponent>().Volume = volume;
    }
    CH_ADD_INTERNAL_CALL(AudioComponent, AudioComponent_SetVolume_Ptr, AudioComponent_SetVolume);

    CH_SCRIPT_FUNC void AudioComponent_SetLoop(uint64_t entityID, bool loop) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<AudioComponent>()) entity.GetComponent<AudioComponent>().Loop = loop;
    }
    CH_ADD_INTERNAL_CALL(AudioComponent, AudioComponent_SetLoop_Ptr, AudioComponent_SetLoop);

    CH_SCRIPT_FUNC bool AudioComponent_IsPlaying(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        if (!entity || !entity.HasComponent<AudioComponent>())
            return false;

        auto& audio = entity.GetComponent<AudioComponent>();
        return audio.IsPlaying && Audio::Get().IsPlaying(audio.SoundHandle);
    }
    CH_ADD_INTERNAL_CALL(AudioComponent, AudioComponent_IsPlaying_Ptr, AudioComponent_IsPlaying);

    CH_SCRIPT_FUNC Coral::String AudioComponent_GetSoundPath(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<AudioComponent>() ? Coral::String::New(entity.GetComponent<AudioComponent>().SoundPath) : Coral::String::New("");
    }
    CH_ADD_INTERNAL_CALL(AudioComponent, AudioComponent_GetSoundPath_Ptr, AudioComponent_GetSoundPath);

    // --- SpriteComponent ---
    CH_SCRIPT_FUNC Coral::String SpriteComponent_GetTexturePath(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<SpriteComponent>() ? Coral::String::New(entity.GetComponent<SpriteComponent>().TexturePath) : Coral::String::New("");
    }
    CH_ADD_INTERNAL_CALL(SpriteComponent, SpriteComponent_GetTexturePath_Ptr, SpriteComponent_GetTexturePath);

    CH_SCRIPT_FUNC void SpriteComponent_SetTexturePath(uint64_t entityID, Coral::String path) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<SpriteComponent>()) {
            auto& comp = entity.GetComponent<SpriteComponent>();
            comp.TexturePath = (std::string)path;
            comp.TextureHandle = 0; // Trigger reload
        }
    }
    CH_ADD_INTERNAL_CALL(SpriteComponent, SpriteComponent_SetTexturePath_Ptr, SpriteComponent_SetTexturePath);

    CH_SCRIPT_FUNC void SpriteComponent_GetTint(uint64_t entityID, glm::vec4* outTint) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<SpriteComponent>() && outTint) {
            auto& tint = entity.GetComponent<SpriteComponent>().Tint;
            *outTint = { tint.r / 255.0f, tint.g / 255.0f, tint.b / 255.0f, tint.a / 255.0f };
        }
    }
    CH_ADD_INTERNAL_CALL(SpriteComponent, SpriteComponent_GetTint_Ptr, SpriteComponent_GetTint);

    CH_SCRIPT_FUNC void SpriteComponent_SetTint(uint64_t entityID, glm::vec4 tint) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<SpriteComponent>()) {
            entity.GetComponent<SpriteComponent>().Tint = { (uint8_t)(tint.r * 255), (uint8_t)(tint.g * 255), (uint8_t)(tint.b * 255), (uint8_t)(tint.a * 255) };
        }
    }
    CH_ADD_INTERNAL_CALL(SpriteComponent, SpriteComponent_SetTint_Ptr, SpriteComponent_SetTint);

    CH_SCRIPT_FUNC bool SpriteComponent_GetFlipX(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<SpriteComponent>() ? entity.GetComponent<SpriteComponent>().FlipX : false;
    }
    CH_ADD_INTERNAL_CALL(SpriteComponent, SpriteComponent_GetFlipX_Ptr, SpriteComponent_GetFlipX);

    CH_SCRIPT_FUNC void SpriteComponent_SetFlipX(uint64_t entityID, bool flip) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<SpriteComponent>()) entity.GetComponent<SpriteComponent>().FlipX = flip;
    }
    CH_ADD_INTERNAL_CALL(SpriteComponent, SpriteComponent_SetFlipX_Ptr, SpriteComponent_SetFlipX);

    CH_SCRIPT_FUNC bool SpriteComponent_GetFlipY(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<SpriteComponent>() ? entity.GetComponent<SpriteComponent>().FlipY : false;
    }
    CH_ADD_INTERNAL_CALL(SpriteComponent, SpriteComponent_GetFlipY_Ptr, SpriteComponent_GetFlipY);

    CH_SCRIPT_FUNC void SpriteComponent_SetFlipY(uint64_t entityID, bool flip) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<SpriteComponent>()) entity.GetComponent<SpriteComponent>().FlipY = flip;
    }
    CH_ADD_INTERNAL_CALL(SpriteComponent, SpriteComponent_SetFlipY_Ptr, SpriteComponent_SetFlipY);

    CH_SCRIPT_FUNC int SpriteComponent_GetZOrder(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<SpriteComponent>() ? entity.GetComponent<SpriteComponent>().ZOrder : 0;
    }
    CH_ADD_INTERNAL_CALL(SpriteComponent, SpriteComponent_GetZOrder_Ptr, SpriteComponent_GetZOrder);

    CH_SCRIPT_FUNC void SpriteComponent_SetZOrder(uint64_t entityID, int z) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<SpriteComponent>()) entity.GetComponent<SpriteComponent>().ZOrder = z;
    }
    CH_ADD_INTERNAL_CALL(SpriteComponent, SpriteComponent_SetZOrder_Ptr, SpriteComponent_SetZOrder);

} // namespace CHEngine

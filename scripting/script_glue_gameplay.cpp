#include "script_glue_internal.h"

namespace CHEngine {

    // ── Spawn / Transition ────────────────────────────────────────────────
    CH_SCRIPT_FUNC bool SpawnComponent_IsActive(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<SpawnComponent>() ? entity.GetComponent<SpawnComponent>().IsActive : false;
    }

    CH_SCRIPT_FUNC void SpawnComponent_GetSpawnPoint(uint64_t entityID, glm::vec3* point) {
        Entity entity = GetEntity(entityID);
        if (!point) return;
        if (entity && entity.HasComponent<SpawnComponent>())
            *point = entity.GetComponent<SpawnComponent>().SpawnPoint;
        else
            *point = {0,0,0};
    }

    CH_SCRIPT_FUNC Coral::String SceneTransitionComponent_GetTargetScene(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<SceneTransitionComponent>())
            return Coral::String::New(entity.GetComponent<SceneTransitionComponent>().TargetScenePath);
        return Coral::String::New("");
    }

    // ── RPGStatsComponent ────────────────────────────────────────────────
    CH_SCRIPT_FUNC int RPGStatsComponent_GetLevel(uint64_t entityID) {
        auto e = GetEntity(entityID);
        return e && e.HasComponent<RPGStatsComponent>() ? e.GetComponent<RPGStatsComponent>().Level : 0;
    }
    CH_SCRIPT_FUNC void RPGStatsComponent_SetLevel(uint64_t entityID, int val) {
        auto e = GetEntity(entityID);
        if (e && e.HasComponent<RPGStatsComponent>()) e.GetComponent<RPGStatsComponent>().Level = val;
    }
    CH_SCRIPT_FUNC float RPGStatsComponent_GetHealth(uint64_t entityID) {
        auto e = GetEntity(entityID);
        return e && e.HasComponent<RPGStatsComponent>() ? e.GetComponent<RPGStatsComponent>().Health : 0.0f;
    }
    CH_SCRIPT_FUNC void RPGStatsComponent_SetHealth(uint64_t entityID, float val) {
        auto e = GetEntity(entityID);
        if (e && e.HasComponent<RPGStatsComponent>()) e.GetComponent<RPGStatsComponent>().Health = val;
    }
    CH_SCRIPT_FUNC int RPGStatsComponent_GetGold(uint64_t entityID) {
        auto e = GetEntity(entityID);
        return e && e.HasComponent<RPGStatsComponent>() ? e.GetComponent<RPGStatsComponent>().Gold : 0;
    }
    CH_SCRIPT_FUNC void RPGStatsComponent_SetGold(uint64_t entityID, int val) {
        auto e = GetEntity(entityID);
        if (e && e.HasComponent<RPGStatsComponent>()) e.GetComponent<RPGStatsComponent>().Gold = val;
    }

    // ── SkillComponent ───────────────────────────────────────────────────
    CH_SCRIPT_FUNC bool SkillComponent_IsUnlocked(uint64_t entityID) {
        auto e = GetEntity(entityID);
        return e && e.HasComponent<SkillComponent>() ? e.GetComponent<SkillComponent>().IsUnlocked : false;
    }
    CH_SCRIPT_FUNC void SkillComponent_SetUnlocked(uint64_t entityID, bool val) {
        auto e = GetEntity(entityID);
        if (e && e.HasComponent<SkillComponent>()) e.GetComponent<SkillComponent>().IsUnlocked = val;
    }

    // ── PlayerComponent ───────────────────────────────────────────────────
    CH_SCRIPT_FUNC float PlayerComponent_GetMovementSpeed(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<PlayerComponent>() ? entity.GetComponent<PlayerComponent>().MovementSpeed : 0.0f;
    }

    CH_SCRIPT_FUNC void PlayerComponent_SetMovementSpeed(uint64_t entityID, float speed) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<PlayerComponent>()) 
            entity.GetComponent<PlayerComponent>().MovementSpeed = speed;
    }

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

    CH_SCRIPT_FUNC void Audio_StopAll() {
        if (Project::GetActive() != nullptr) {
            Audio::Get().StopAll();
        }
    }

    CH_SCRIPT_FUNC void AudioComponent_SetVolume(uint64_t entityID, float volume) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<AudioComponent>()) entity.GetComponent<AudioComponent>().Volume = volume;
    }

    CH_SCRIPT_FUNC void AudioComponent_SetLoop(uint64_t entityID, bool loop) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<AudioComponent>()) entity.GetComponent<AudioComponent>().Loop = loop;
    }

    CH_SCRIPT_FUNC bool AudioComponent_IsPlaying(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        if (!entity || !entity.HasComponent<AudioComponent>())
            return false;

        auto& audio = entity.GetComponent<AudioComponent>();
        return audio.IsPlaying && Audio::Get().IsPlaying(audio.SoundHandle);
    }

    CH_SCRIPT_FUNC Coral::String AudioComponent_GetSoundPath(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<AudioComponent>() ? Coral::String::New(entity.GetComponent<AudioComponent>().SoundPath) : Coral::String::New("");
    }

    void RegisterGameplayInternalCalls(Coral::ManagedAssembly& assembly) {
        #define CH_ADD_INTERNAL_CALL(className, fieldName, funcPtr) assembly.AddInternalCall("CHEngine." #className, #fieldName, (void*)funcPtr)
        
        CH_ADD_INTERNAL_CALL(Audio, Audio_Play_Ptr, Audio_Play);
        CH_ADD_INTERNAL_CALL(Audio, Audio_Stop_Ptr, Audio_Stop);
        CH_ADD_INTERNAL_CALL(Audio, Audio_StopAll_Ptr, Audio_StopAll);
        CH_ADD_INTERNAL_CALL(PlayerComponent, PlayerComponent_GetMovementSpeed_Ptr, PlayerComponent_GetMovementSpeed);
        CH_ADD_INTERNAL_CALL(PlayerComponent, PlayerComponent_SetMovementSpeed_Ptr, PlayerComponent_SetMovementSpeed);
        CH_ADD_INTERNAL_CALL(AudioComponent, AudioComponent_SetVolume_Ptr, AudioComponent_SetVolume);
        CH_ADD_INTERNAL_CALL(AudioComponent, AudioComponent_SetLoop_Ptr, AudioComponent_SetLoop);
        CH_ADD_INTERNAL_CALL(AudioComponent, AudioComponent_IsPlaying_Ptr, AudioComponent_IsPlaying);
        CH_ADD_INTERNAL_CALL(AudioComponent, AudioComponent_GetSoundPath_Ptr, AudioComponent_GetSoundPath);
        CH_ADD_INTERNAL_CALL(SpawnComponent, SpawnComponent_IsActive_Ptr, SpawnComponent_IsActive);
        CH_ADD_INTERNAL_CALL(SpawnComponent, SpawnComponent_GetSpawnPoint_Ptr, SpawnComponent_GetSpawnPoint);
        CH_ADD_INTERNAL_CALL(SceneTransitionComponent, SceneTransitionComponent_GetTargetScene_Ptr, SceneTransitionComponent_GetTargetScene);
        
        // RPG
        CH_ADD_INTERNAL_CALL(RPGStatsComponent, RPGStatsComponent_GetLevel_Ptr, RPGStatsComponent_GetLevel);
        CH_ADD_INTERNAL_CALL(RPGStatsComponent, RPGStatsComponent_SetLevel_Ptr, RPGStatsComponent_SetLevel);
        CH_ADD_INTERNAL_CALL(RPGStatsComponent, RPGStatsComponent_GetHealth_Ptr, RPGStatsComponent_GetHealth);
        CH_ADD_INTERNAL_CALL(RPGStatsComponent, RPGStatsComponent_SetHealth_Ptr, RPGStatsComponent_SetHealth);
        CH_ADD_INTERNAL_CALL(RPGStatsComponent, RPGStatsComponent_GetGold_Ptr, RPGStatsComponent_GetGold);
        CH_ADD_INTERNAL_CALL(RPGStatsComponent, RPGStatsComponent_SetGold_Ptr, RPGStatsComponent_SetGold);

        CH_ADD_INTERNAL_CALL(SkillComponent, SkillComponent_IsUnlocked_Ptr, SkillComponent_IsUnlocked);
        CH_ADD_INTERNAL_CALL(SkillComponent, SkillComponent_SetUnlocked_Ptr, SkillComponent_SetUnlocked);

        #undef CH_ADD_INTERNAL_CALL
    }

} // namespace CHEngine

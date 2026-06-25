#include "script_glue_internal.h"
#include "script_internal_call_registry.h"
#include "engine/scene/component_registry.h"
#include "engine/audio/audio.h"
#include "engine/core/service_locator.h"
#include "engine/project/project.h"

namespace Chained {

    void RegisterGlueAudio() {}

    // ── Audio ─────────────────────────────────────────────────────────────
    CH_SCRIPT_FUNC void Audio_Play(Coral::String path, float volume, float pitch, bool loop) {
        if (Project::GetActive() != nullptr) {
            const std::string soundPath = (std::string)path;
            auto* audioService = ServiceLocator::Get<Audio>();

            AudioHandle handle = audioService->LoadSound(soundPath);
            if (handle != 0) {
                audioService->Play(handle, volume, pitch, loop, false, glm::vec3(0));

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
            ServiceLocator::Get<Audio>()->Stop(soundPath);

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
            ServiceLocator::Get<Audio>()->StopAll();
        }
    }
    CH_ADD_INTERNAL_CALL(Audio, Audio_StopAll_Ptr, Audio_StopAll);

    CH_SCRIPT_FUNC void AudioComponent_SetVolume(uint64_t entityID, float volume) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<AudioComponent>()) {
            auto& audio = entity.GetComponent<AudioComponent>();
            audio.Volume = volume;
            if (audio.IsPlaying && audio.SoundHandle != 0) {
                ServiceLocator::Get<Audio>()->SetVolume(audio.SoundHandle, volume);
            }
        }
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
        return audio.IsPlaying && ServiceLocator::Get<Audio>()->IsPlaying(audio.SoundHandle);
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
            comp.TextureHandle = 0;
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

} // namespace Chained

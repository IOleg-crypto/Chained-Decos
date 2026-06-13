#include "script_glue_internal.h"
#include "engine/audio/audio.h"
#include "engine/project/project.h"

namespace Chained {

    

    // ── Audio ─────────────────────────────────────────────────────────────
    CH_SCRIPT_FUNC void Audio_Play(Coral::String path, float volume, float pitch, bool loop) {
        if (Project::GetActive() != nullptr) {
            const std::string soundPath = (std::string)path;
            auto& audio = Audio::Get();

            AudioHandle handle = audio.LoadSound(soundPath);
            if (handle != 0) {
                audio.Play(handle, volume, pitch, loop, false, glm::vec3(0));

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
            auto& audio = Audio::Get();
            audio.Stop(soundPath);

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
            auto& audio = Audio::Get();
            audio.StopAll();
        }
    }
    

    CH_SCRIPT_FUNC void AudioComponent_SetVolume(uint64_t entityID, float volume) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<AudioComponent>()) {
            auto& audio = entity.GetComponent<AudioComponent>();
            audio.Volume = volume;
            // Forward to active sound instance in real-time
            if (audio.IsPlaying && audio.SoundHandle != 0) {
                auto& audioService = Audio::Get();
                audioService.SetVolume(audio.SoundHandle, volume);
            }
        }
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
        auto& audioService = Audio::Get();
        return audio.IsPlaying && audioService.IsPlaying(audio.SoundHandle);
    }
    

    CH_SCRIPT_FUNC Coral::String AudioComponent_GetSoundPath(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<AudioComponent>() ? Coral::String::New(entity.GetComponent<AudioComponent>().SoundPath) : Coral::String::New("");
    }
    

    // --- SpriteComponent ---
    CH_SCRIPT_FUNC Coral::String SpriteComponent_GetTexturePath(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<SpriteComponent>() ? Coral::String::New(entity.GetComponent<SpriteComponent>().TexturePath) : Coral::String::New("");
    }
    

    CH_SCRIPT_FUNC void SpriteComponent_SetTexturePath(uint64_t entityID, Coral::String path) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<SpriteComponent>()) {
            auto& comp = entity.GetComponent<SpriteComponent>();
            comp.TexturePath = (std::string)path;
            comp.TextureHandle = 0; // Trigger reload
        }
    }
    

    CH_SCRIPT_FUNC void SpriteComponent_GetTint(uint64_t entityID, glm::vec4* outTint) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<SpriteComponent>() && outTint) {
            auto& tint = entity.GetComponent<SpriteComponent>().Tint;
            *outTint = { tint.r / 255.0f, tint.g / 255.0f, tint.b / 255.0f, tint.a / 255.0f };
        }
    }
    

    CH_SCRIPT_FUNC void SpriteComponent_SetTint(uint64_t entityID, glm::vec4 tint) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<SpriteComponent>()) {
            entity.GetComponent<SpriteComponent>().Tint = { (uint8_t)(tint.r * 255), (uint8_t)(tint.g * 255), (uint8_t)(tint.b * 255), (uint8_t)(tint.a * 255) };
        }
    }
    

    CH_SCRIPT_FUNC bool SpriteComponent_GetFlipX(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<SpriteComponent>() ? entity.GetComponent<SpriteComponent>().FlipX : false;
    }
    

    CH_SCRIPT_FUNC void SpriteComponent_SetFlipX(uint64_t entityID, bool flip) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<SpriteComponent>()) entity.GetComponent<SpriteComponent>().FlipX = flip;
    }
    

    CH_SCRIPT_FUNC bool SpriteComponent_GetFlipY(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<SpriteComponent>() ? entity.GetComponent<SpriteComponent>().FlipY : false;
    }
    

    CH_SCRIPT_FUNC void SpriteComponent_SetFlipY(uint64_t entityID, bool flip) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<SpriteComponent>()) entity.GetComponent<SpriteComponent>().FlipY = flip;
    }
    

    CH_SCRIPT_FUNC int SpriteComponent_GetZOrder(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<SpriteComponent>() ? entity.GetComponent<SpriteComponent>().ZOrder : 0;
    }
    

    CH_SCRIPT_FUNC void SpriteComponent_SetZOrder(uint64_t entityID, int z) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<SpriteComponent>()) entity.GetComponent<SpriteComponent>().ZOrder = z;
    }
    

        void RegisterGlueAudio(Coral::ManagedAssembly& assembly) {
            assembly.AddInternalCall("Chained.Audio", "Audio_Play_Ptr", (void*)Audio_Play);
            assembly.AddInternalCall("Chained.Audio", "Audio_Stop_Ptr", (void*)Audio_Stop);
            assembly.AddInternalCall("Chained.Audio", "Audio_StopAll_Ptr", (void*)Audio_StopAll);
        }
} // namespace Chained

#include "script_glue_audio.h"
namespace Chained {
void Audio_Play(Coral::String path, float volume, float pitch, bool loop)
{
    if (Project::GetActive() != nullptr)
    {
        const std::string soundPath = (std::string)path;
        auto* audioService = ServiceLocator::Get<Audio>();

        AudioHandle handle = audioService->LoadSound(soundPath);
        if (handle != 0)
        {
            audioService->Play(handle, volume, pitch, loop, false, glm::vec3(0));

            if (Scene* scene = GetActiveScene())
            {
                auto& registry = scene->GetRegistry();
                auto view = registry.view<AudioComponent>();
                for (auto entity : view)
                {
                    auto& audio = view.get<AudioComponent>(entity);
                    if (audio.SoundPath == soundPath)
                    {
                        audio.SoundHandle = handle;
                        audio.IsPlaying = true;
                    }
                }
            }
        }
    }
}
void Audio_Stop(Coral::String path)
{
    if (Project::GetActive() != nullptr)
    {
        const std::string soundPath = (std::string)path;
        ServiceLocator::Get<Audio>()->Stop(soundPath);

        if (Scene* scene = GetActiveScene())
        {
            auto& registry = scene->GetRegistry();
            auto view = registry.view<AudioComponent>();
            for (auto entity : view)
            {
                auto& audio = view.get<AudioComponent>(entity);
                if (audio.SoundPath == soundPath)
                {
                    audio.IsPlaying = false;
                }
            }
        }
    }
}
void Audio_StopAll()
{
    if (Project::GetActive() != nullptr)
    {
        ServiceLocator::Get<Audio>()->StopAll();
    }
}
void AudioComponent_SetVolume(uint64_t entityID, float volume)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<AudioComponent>())
    {
        auto& audio = entity.GetComponent<AudioComponent>();
        audio.Volume = volume;
        if (audio.IsPlaying && audio.SoundHandle != 0)
        {
            ServiceLocator::Get<Audio>()->SetVolume(audio.SoundHandle, volume);
        }
    }
}
void AudioComponent_SetLoop(uint64_t entityID, bool loop)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<AudioComponent>())
    {
        entity.GetComponent<AudioComponent>().Loop = loop;
    }
}
bool AudioComponent_IsPlaying(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    if (!entity || !entity.HasComponent<AudioComponent>())
    {
        return false;
    }

    auto& audio = entity.GetComponent<AudioComponent>();
    return audio.IsPlaying && ServiceLocator::Get<Audio>()->IsPlaying(audio.SoundHandle);
}
Coral::String AudioComponent_GetSoundPath(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<AudioComponent>()
               ? Coral::String::New(entity.GetComponent<AudioComponent>().SoundPath)
               : Coral::String::New("");
}
Coral::String SpriteComponent_GetTexturePath(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<SpriteComponent>()
               ? Coral::String::New(entity.GetComponent<SpriteComponent>().TexturePath)
               : Coral::String::New("");
}
void SpriteComponent_SetTexturePath(uint64_t entityID, Coral::String path)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<SpriteComponent>())
    {
        auto& comp = entity.GetComponent<SpriteComponent>();
        comp.TexturePath = (std::string)path;
        comp.TextureHandle = 0;
    }
}
void SpriteComponent_GetTint(uint64_t entityID, glm::vec4* outTint)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<SpriteComponent>() && outTint)
    {
        auto& tint = entity.GetComponent<SpriteComponent>().Tint;
        *outTint = {tint.r / 255.0f, tint.g / 255.0f, tint.b / 255.0f, tint.a / 255.0f};
    }
}
void SpriteComponent_SetTint(uint64_t entityID, glm::vec4 tint)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<SpriteComponent>())
    {
        entity.GetComponent<SpriteComponent>().Tint = {(uint8_t)(tint.r * 255), (uint8_t)(tint.g * 255),
                                                       (uint8_t)(tint.b * 255), (uint8_t)(tint.a * 255)};
    }
}
bool SpriteComponent_GetFlipX(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<SpriteComponent>() ? entity.GetComponent<SpriteComponent>().FlipX : false;
}
void SpriteComponent_SetFlipX(uint64_t entityID, bool flip)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<SpriteComponent>())
    {
        entity.GetComponent<SpriteComponent>().FlipX = flip;
    }
}
bool SpriteComponent_GetFlipY(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<SpriteComponent>() ? entity.GetComponent<SpriteComponent>().FlipY : false;
}
void SpriteComponent_SetFlipY(uint64_t entityID, bool flip)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<SpriteComponent>())
    {
        entity.GetComponent<SpriteComponent>().FlipY = flip;
    }
}
int SpriteComponent_GetZOrder(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<SpriteComponent>() ? entity.GetComponent<SpriteComponent>().ZOrder : 0;
}
void SpriteComponent_SetZOrder(uint64_t entityID, int z)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<SpriteComponent>())
    {
        entity.GetComponent<SpriteComponent>().ZOrder = z;
    }
}

void RegisterGlueAudio()
{
    CH_ADD_INTERNAL_CALL("Audio", Audio_Play, Audio_Play);
    CH_ADD_INTERNAL_CALL("Audio", Audio_Stop, Audio_Stop);
    CH_ADD_INTERNAL_CALL("Audio", Audio_StopAll, Audio_StopAll);
    CH_ADD_INTERNAL_CALL("Audio", AudioComponent_SetVolume, AudioComponent_SetVolume);
    CH_ADD_INTERNAL_CALL("Audio", AudioComponent_SetLoop, AudioComponent_SetLoop);
    CH_ADD_INTERNAL_CALL("Audio", AudioComponent_IsPlaying, AudioComponent_IsPlaying);
    CH_ADD_INTERNAL_CALL("Audio", AudioComponent_GetSoundPath, AudioComponent_GetSoundPath);
    CH_ADD_INTERNAL_CALL("Audio", SpriteComponent_GetTexturePath, SpriteComponent_GetTexturePath);
    CH_ADD_INTERNAL_CALL("Audio", SpriteComponent_SetTexturePath, SpriteComponent_SetTexturePath);
    CH_ADD_INTERNAL_CALL("Audio", SpriteComponent_GetTint, SpriteComponent_GetTint);
    CH_ADD_INTERNAL_CALL("Audio", SpriteComponent_SetTint, SpriteComponent_SetTint);
    CH_ADD_INTERNAL_CALL("Audio", SpriteComponent_GetFlipX, SpriteComponent_GetFlipX);
    CH_ADD_INTERNAL_CALL("Audio", SpriteComponent_SetFlipX, SpriteComponent_SetFlipX);
    CH_ADD_INTERNAL_CALL("Audio", SpriteComponent_GetFlipY, SpriteComponent_GetFlipY);
    CH_ADD_INTERNAL_CALL("Audio", SpriteComponent_SetFlipY, SpriteComponent_SetFlipY);
    CH_ADD_INTERNAL_CALL("Audio", SpriteComponent_GetZOrder, SpriteComponent_GetZOrder);
    CH_ADD_INTERNAL_CALL("Audio", SpriteComponent_SetZOrder, SpriteComponent_SetZOrder);
}
}

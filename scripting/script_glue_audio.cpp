#include "script_glue_audio.h"
#include "engine/scene/components.h"
#include <algorithm>
namespace Chained
{
static thread_local Coral::UCString s_AudioTagBuffer;

void Audio_Play(const Coral::UCChar* path, float volume, float pitch, bool loop)
{
    if (Project::GetActive() != nullptr && path)
    {
        const std::string soundPath = ch_u16_to_string(path);
        auto* audioService = ServiceLocator::TryGet<Audio>();
        if (!audioService)
        {
            return;
        }

        AssetHandle handle = audioService->LoadSound(soundPath);
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
void Audio_Stop(const Coral::UCChar* path)
{
    if (Project::GetActive() != nullptr && path)
    {
        const std::string soundPath = ch_u16_to_string(path);
        auto* audioService = ServiceLocator::TryGet<Audio>();
        if (!audioService)
        {
            return;
        }
        audioService->Stop(soundPath);

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
        auto* audioService = ServiceLocator::TryGet<Audio>();
        if (!audioService)
        {
            return;
        }
        audioService->StopAll();
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
            auto* audioService = ServiceLocator::TryGet<Audio>();
            if (audioService)
            {
                audioService->SetVolume(audio.SoundHandle, volume);
            }
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
    auto* audioService = ServiceLocator::TryGet<Audio>();
    if (!audioService)
    {
        return false;
    }
    return audio.IsPlaying && audioService->IsPlaying(audio.SoundHandle);
}
const Coral::UCChar* AudioComponent_GetSoundPath(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    std::string path =
        entity && entity.HasComponent<AudioComponent>() ? entity.GetComponent<AudioComponent>().SoundPath : "";
    s_AudioTagBuffer = ToWide(path);
    return s_AudioTagBuffer.c_str();
}
const Coral::UCChar* SpriteComponent_GetTexturePath(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    std::string path =
        entity && entity.HasComponent<SpriteComponent>() ? entity.GetComponent<SpriteComponent>().TexturePath : "";
    s_AudioTagBuffer = ToWide(path);
    return s_AudioTagBuffer.c_str();
}
void SpriteComponent_SetTexturePath(uint64_t entityID, const Coral::UCChar* path)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<SpriteComponent>() && path)
    {
        auto& comp = entity.GetComponent<SpriteComponent>();
        comp.TexturePath = ch_u16_to_string(path);
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
        auto clamped = [](float v) -> uint8_t { return (uint8_t)(std::clamp(v, 0.0f, 1.0f) * 255); };
        entity.GetComponent<SpriteComponent>().Tint = {clamped(tint.r), clamped(tint.g), clamped(tint.b),
                                                       clamped(tint.a)};
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
} // namespace Chained

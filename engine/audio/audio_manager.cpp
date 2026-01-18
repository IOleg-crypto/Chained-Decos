#include "audio_manager.h"
#include "engine/core/log.h"

namespace CHEngine
{
void AudioManager::Init()
{
    InitAudioDevice();
    if (IsAudioDeviceReady())
    {
        CH_CORE_INFO("Audio Device Initialized Successfully");
    }
    else
    {
        CH_CORE_ERROR("Failed to initialize Audio Device!");
    }
}

void AudioManager::Shutdown()
{
    CloseAudioDevice();
    CH_CORE_INFO("Audio Device Shutdown.");
}

void AudioManager::PlaySound(Ref<SoundAsset> asset, float volume, float pitch)
{
    if (!asset)
    {
        CH_CORE_WARN("Attempted to play null sound asset");
        return;
    }

    Sound &sound = asset->GetSound();
    SetSoundVolume(sound, volume);
    SetSoundPitch(sound, pitch);
    ::PlaySound(sound);
}

void AudioManager::StopSound(Ref<SoundAsset> asset)
{
    if (asset)
    {
        ::StopSound(asset->GetSound());
    }
}

void AudioManager::SetMasterVolume(float volume)
{
    ::SetMasterVolume(volume);
}
} // namespace CHEngine

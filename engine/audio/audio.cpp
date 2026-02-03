#include "audio.h"
#include "sound_asset.h"
#include "engine/scene/components.h"
#include "engine/core/log.h"
#include "raylib.h"

namespace CHEngine
{
    void Audio::Init()
    {
        if (!IsAudioDeviceReady())
        {
            InitAudioDevice();
            CH_CORE_INFO("Audio System Initialized.");
        }
    }

    void Audio::Shutdown()
    {
        if (IsAudioDeviceReady())
        {
            CloseAudioDevice();
            CH_CORE_INFO("Audio System Shutdown.");
        }
    }

    void Audio::Update(Scene* scene, Timestep ts)
    {
        auto& registry = scene->GetRegistry();
        auto view = registry.view<AudioComponent>();

        for (auto entity : view)
        {
            auto& audio = view.get<AudioComponent>(entity);
            if (audio.Asset && audio.Asset->GetState() == AssetState::Ready)
            {
                // Reactive-like update: ensure volume/pitch match component state
                SetSoundVolume(audio.Asset->GetSound(), audio.Volume);
                SetSoundPitch(audio.Asset->GetSound(), audio.Pitch);

                // Handle looping (Raylib Sound doesn't have a simple loop flag in PlaySound, 
                // so we check if finished and restart if looping is desired)
                if (audio.Loop && !IsSoundPlaying(audio.Asset->GetSound()) && audio.IsPlaying)
                {
                    PlaySound(audio.Asset->GetSound());
                }
            }
        }
    }

    void Audio::Play(std::shared_ptr<SoundAsset> asset, float volume, float pitch, bool loop)
    {
        if (asset && asset->GetState() == AssetState::Ready)
        {
            SetSoundVolume(asset->GetSound(), volume);
            SetSoundPitch(asset->GetSound(), pitch);
            PlaySound(asset->GetSound());
        }
    }

    void Audio::Stop(std::shared_ptr<SoundAsset> asset)
    {
        if (asset && asset->GetState() == AssetState::Ready)
        {
            StopSound(asset->GetSound());
        }
    }
}

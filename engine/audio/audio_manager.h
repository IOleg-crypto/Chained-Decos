#ifndef CH_AUDIO_MANAGER_H
#define CH_AUDIO_MANAGER_H

#include "engine/core/base.h"
#include "sound_asset.h"
#include <string>

namespace CHEngine
{
class AudioManager
{
public:
    static void Init();
    static void Shutdown();

    static void PlaySound(Ref<SoundAsset> asset, float volume = 1.0f, float pitch = 1.0f);
    static void StopSound(Ref<SoundAsset> asset);

    static void SetMasterVolume(float volume);
};
} // namespace CHEngine

#endif // CH_AUDIO_MANAGER_H

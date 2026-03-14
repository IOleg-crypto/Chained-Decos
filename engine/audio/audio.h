#ifndef CH_AUDIO_H
#define CH_AUDIO_H

#include "engine/core/base.h"
#include "engine/core/timestep.h"
#include "engine/scene/scene.h"
#include <memory>

namespace CHEngine
{
class SoundAsset;

// Stateless action class for global audio management.
class Audio
{
public:
    Audio();
    ~Audio();

    // Initializes the audio backend.
    void Init();

    // Shuts down the audio backend.
    void Shutdown();

    // Updates all active audio sources in the scene.
    void Update(Scene* scene, Timestep ts);

    // Plays a specified sound asset.
    void Play(std::shared_ptr<SoundAsset> asset, float volume = 1.0f, float pitch = 1.0f, bool loop = false);

    // Stops a specified sound asset.
    void Stop(std::shared_ptr<SoundAsset> asset);

    static Audio& Get();
};
} // namespace CHEngine

#endif // CH_AUDIO_H

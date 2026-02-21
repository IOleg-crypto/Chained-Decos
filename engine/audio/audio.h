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
    // Initializes the audio backend.
    static void Init();

    // Shuts down the audio backend.
    static void Shutdown();

    // Updates all active audio sources in the scene.
    static void Update(Scene* scene, Timestep ts);

    // Plays a specified sound asset.
    static void Play(std::shared_ptr<SoundAsset> asset, float volume = 1.0f, float pitch = 1.0f, bool loop = false);

    // Stops a specified sound asset.
    static void Stop(std::shared_ptr<SoundAsset> asset);
};
} // namespace CHEngine

#endif // CH_AUDIO_H

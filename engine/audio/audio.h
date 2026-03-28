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
    static void Init();

    // Shuts down the audio backend.
    static void Shutdown();

    void InternalInit();
    void InternalShutdown();

    // Updates all active audio sources in the scene.
    void Update(Scene* scene, Timestep ts);

    static Audio& Get();
    void* GetEngine() { return m_Engine; }

    void SetListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);

    // Plays a specified sound asset.
    void Play(std::shared_ptr<SoundAsset> asset, float volume = 1.0f, float pitch = 1.0f, bool loop = false, bool spatial = false, const glm::vec3& pos = {0,0,0});

    // Stops a specified sound asset.
    void Stop(std::shared_ptr<SoundAsset> asset);

private:
    void* m_Engine = nullptr;
};

} // namespace CHEngine

#endif // CH_AUDIO_H

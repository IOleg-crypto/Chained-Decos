#ifndef CH_AUDIO_H
#define CH_AUDIO_H

#include "engine/core/timestep.h"
#include <glm/glm.hpp>
#include <memory>
#include <mutex>
#include <vector>

#include "miniaudio.h"

namespace CHEngine
{
struct AudioBuffer
{
    const float* Data = nullptr;
    uint32_t Size = 0; // Total count of floats (frames * channels)
    uint32_t Channels = 0;
    uint32_t SampleRate = 0;
};
struct SoundInstance
{
    ma_sound Sound;
    ma_audio_buffer Buffer;
};

class Audio
{
public:
    // Access the global audio system instance (Meyers Singleton)
    static Audio& Get()
    {
        static Audio instance;
        return instance;
    }

    // Deleted constructors for singleton
    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    // Optional: for future use (per-frame updates)
    void Update(Timestep ts);

    void SetListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);

    // Plays a sound from a raw audio buffer
    void Play(const AudioBuffer& buffer, float volume = 1.0f, float pitch = 1.0f, bool loop = false,
              bool spatial = false, const glm::vec3& pos = {0, 0, 0});

    // Stop all audio
    void StopAll();

private:
    Audio();
    ~Audio();

private:
    void* m_Engine = nullptr; // ma_engine*
};

} // namespace CHEngine

#endif // CH_AUDIO_H

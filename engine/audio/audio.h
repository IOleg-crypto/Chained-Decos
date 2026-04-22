#ifndef CH_AUDIO_H
#define CH_AUDIO_H

#include "engine/core/timestep.h"
#include "engine/core/uuid.h" // Assuming AssetHandle / UUID is here. If not, include where AssetHandle is.
#include <glm/glm.hpp>
#include <memory>
#include <miniaudio.h>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace CHEngine
{
using AudioHandle = UUID;

struct SoundInstance
{
    ma_sound Sound;
    ma_audio_buffer Buffer;
    AudioHandle Handle;
};
struct AudioData
{
    std::vector<float> PCMData;
    uint32_t Channels = 0;
    uint32_t SampleRate = 0;

    // For play usage inside Audio
    const float* Data() const
    {
        return PCMData.data();
    }
    uint32_t Size() const
    {
        return (uint32_t)PCMData.size();
    }
};

class Audio
{
public:
    Audio();
    ~Audio();

    static Audio& Get();

    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    void Update(Timestep ts);
    void SetListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);

    // Loads audio from disk synchronously. Returns a Handle that is fast & safe to use.
    AudioHandle LoadSound(const std::string& filepath);
    // Returns true if handle is valid and data is loaded. (0 is invalid handle in Hazel style)
    bool IsSoundLoaded(AudioHandle handle) const;
    bool IsPlaying(AudioHandle handle) const;

    // Plays a sound.
    void Play(AudioHandle handle, float volume = 1.0f, float pitch = 1.0f, bool loop = false, bool spatial = false,
              const glm::vec3& pos = {0, 0, 0});

    // Stops any active instance that was loaded from the given path.
    void Stop(const std::string& filepath);
    void Stop(AudioHandle handle);

    void StopAll();

private:
    ma_engine* m_Engine = nullptr;

    mutable std::mutex m_DataMutex;
    std::unordered_map<AudioHandle, AudioData> m_AudioDataRegistry;
    std::unordered_map<std::string, AudioHandle> m_PathRegistry;
};

} // namespace CHEngine

#endif // CH_AUDIO_H

#ifndef CH_AUDIO_H
#define CH_AUDIO_H

#include "engine/core/base.h"
#include "engine/core/timestep.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace CHEngine
{
struct ListenerData
{
    glm::vec3 Position = {0, 0, 0};
    glm::vec3 Forward = {0, 0, -1};
    glm::vec3 Up = {0, 1, 0};
};

struct SoundInstance
{
    void* Decoder = nullptr;
    std::string Path;
    glm::vec3 Position = {0, 0, 0};
    float Volume = 1.0f;
    float Pitch = 1.0f;
    bool Loop = false;
    bool Spatial = false;
    bool IsInitialized = false;
    bool Finished = false;
};

class Audio
{
public:
    Audio();
    ~Audio();

    // Initializes the audio device (WASAPI/DirectSound/etc)
    static void Init();

    // Shuts down the audio device
    static void Shutdown();

    // Cleans up finished sounds (call once per frame)
    void Update(Timestep ts);

    static Audio& Get();

    void SetListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);

    // Plays a sound from a file path.
    void Play(const std::string& filepath, float volume = 1.0f, float pitch = 1.0f, bool loop = false,
              bool spatial = false, const glm::vec3& pos = {0, 0, 0});

    // Stops all instances of a sound with the given path
    void Stop(const std::string& filepath);

    // Stop all audio
    void StopAll();

private:
    static void DataCallback(void* pDevice, void* pOutput, const void* pInput, unsigned int frameCount);

private:
    void* m_Device = nullptr; // ma_device*
private:
    ListenerData m_Listener;
    std::vector<std::shared_ptr<SoundInstance>> m_ActiveSounds;
    std::mutex m_AudioMutex;
};

} // namespace CHEngine

#endif // CH_AUDIO_H

#ifndef CH_AUDIO_H
#define CH_AUDIO_H


#include "engine/foundation/timestep.h"
#include "engine/foundation/uuid.h"
#include <glm/glm.hpp>
#include <memory>
#include <miniaudio.h>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>


namespace Chained
{
using AudioHandle = UUID;

struct SoundInstance
{
    ma_sound Sound;
    AudioHandle Handle;
};

class Audio
{
public:
    static void Init();
    static void Shutdown();
    static Audio& Get();

private:
    Audio();
    ~Audio();
public:
    void SetListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);

public:
    AudioHandle LoadSound(const std::string& filepath);
    bool IsSoundLoaded(AudioHandle handle) const;
    bool IsPlaying(AudioHandle handle) const;

    void Play(AudioHandle handle, float volume = 1.0f, float pitch = 1.0f, bool loop = false, bool spatial = false,
              const glm::vec3& pos = {0, 0, 0});

    void SetInstancePosition(AudioHandle handle, const glm::vec3& pos);
    void SetVolume(AudioHandle handle, float volume);
    void SetPitch(AudioHandle handle, float pitch);

    void Stop(const std::string& filepath);
    void Stop(AudioHandle handle);
    void StopAll();
    void Update(Timestep ts);

public:
    ma_engine* GetEngine() const;
private:
    ma_engine* m_Engine = nullptr;
private:

    mutable std::mutex m_DataMutex;
    std::vector<std::unique_ptr<SoundInstance>> m_ActiveSounds;
    std::unordered_map<std::string, AudioHandle> m_PathToHandle;
    std::unordered_map<AudioHandle, std::string> m_HandleToPath;

    static inline Audio* s_Instance = nullptr;
};

} // namespace Chained

#endif // CH_AUDIO_H

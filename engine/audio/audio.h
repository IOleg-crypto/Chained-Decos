#ifndef CH_AUDIO_H
#define CH_AUDIO_H


#include "engine/common/timestep.h"
#include "engine/common/uuid.h"
#include <glm/glm.hpp>
#include <memory>
#include <miniaudio.h>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "engine/core/engine_module.h"


namespace Chained
{
using AudioHandle = UUID;

struct SoundInstance
{
    ma_sound Sound;
    AudioHandle Handle;
};

class Audio : public EngineModule
{
public:
    virtual void Initialize() override;
    virtual void Update(Timestep ts) override;
    virtual void Shutdown() override;

public:
    Audio();
    virtual ~Audio() override;
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

public:
    ma_engine* GetEngine() const;
private:
    ma_engine* m_Engine = nullptr;
private:

    mutable std::mutex m_DataMutex;
    std::vector<std::unique_ptr<SoundInstance>> m_ActiveSounds;
    std::unordered_map<std::string, AudioHandle> m_PathToHandle;
    std::unordered_map<AudioHandle, std::string> m_HandleToPath;
};

} // namespace Chained

#endif // CH_AUDIO_H

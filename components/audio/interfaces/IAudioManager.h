#ifndef IAUDIO_MANAGER_H
#define IAUDIO_MANAGER_H

#include <string>

class IAudioManager
{
public:
    virtual ~IAudioManager() = default;

    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void SetMasterVolume(float volume) = 0;

    // Sound effects
    virtual bool LoadSound(const std::string &name, const std::string &filePath) = 0;
    virtual void PlaySoundEffect(const std::string &name, float volume = 5.0f,
                                 float pitch = 1.0f) = 0;
    virtual void PlayLoopingSoundEffect(const std::string &name, float volume = 1.0f,
                                        float pitch = 1.0f) = 0;
    virtual void StopLoopingSoundEffect(const std::string &name) = 0;
    virtual void UpdateLoopingSounds() = 0;
};

#endif // IAUDIO_MANAGER_H

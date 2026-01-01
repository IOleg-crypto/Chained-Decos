#ifndef AUDIO_H
#define AUDIO_H

#include <string>

namespace CHEngine
{

class Audio
{
public:
    static void Init();
    static void Shutdown();
    static bool IsInitialized();
    static void Update(float deltaTime);

    static bool LoadSound(const std::string &name, const std::string &filePath);
    static void PlaySoundEffect(const std::string &name, float volume = 5.0f, float pitch = 1.0f);
    static void PlayLoopingSoundEffect(const std::string &name, float volume = 1.0f,
                                       float pitch = 1.0f);
    static void StopLoopingSoundEffect(const std::string &name);
    static void UpdateLoopingSounds();

    static void PlayMusic(const std::string &name, float volume = 1.0f);
    static void StopMusic();
    static void PauseMusic();
    static void ResumeMusic();

    static void SetMasterVolume(float volume);
    static void SetMusicVolume(float volume);
    static void SetSoundVolume(float volume);
};

} // namespace CHEngine

#endif // AUDIO_H

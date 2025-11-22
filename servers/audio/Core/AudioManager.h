#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include "../Interfaces/IAudioManager.h"
#include <memory>
#include <raylib.h>
#include <string>
#include <unordered_map>

#include "../../Kernel/Interfaces/IKernelService.h"

class AudioManager : public IAudioManager, public IKernelService
{
public:
    AudioManager();
    ~AudioManager() override;

    // Initialize audio system
    bool Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override
    {
        UpdateLoopingSounds();
    }
    void Render() override
    {
    }
    const char *GetName() const override
    {
        return "AudioManager";
    }

    // Load audio files
    bool LoadSound(const std::string &name, const std::string &filePath);
    bool LoadMusic(const std::string &name, const std::string &filePath);

    // Play audio
    void PlaySoundEffect(const std::string &name, float volume = 5.0f, float pitch = 1.0f);
    void PlayLoopingSoundEffect(const std::string &name, float volume = 1.0f, float pitch = 1.0f);
    void StopLoopingSoundEffect(const std::string &name);
    void UpdateLoopingSounds();
    void PlayMusic(const std::string &name, float volume = 1.0f);
    void StopMusic();

    // Control music playback
    void PauseMusic();
    void ResumeMusic();
    bool IsMusicPlaying() const;

    // Volume control
    void SetMasterVolume(float volume) override;
    void SetMusicVolume(float volume);
    void SetSoundVolume(float volume);

    // Cleanup
    void UnloadSound(const std::string &name);
    void UnloadMusic(const std::string &name);
    void UnloadAll();

private:
    std::unordered_map<std::string, Sound> m_sounds;
    std::unordered_map<std::string, Music> m_music;
    std::unordered_map<std::string, bool> m_loopingSounds;
    Music m_currentMusic;
    bool m_musicPlaying;

    float m_masterVolume;
    float m_musicVolume;
    float m_soundVolume;
};

#endif // AUDIOMANAGER_H
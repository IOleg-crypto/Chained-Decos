#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <raylib.h>
#include <string>
#include <unordered_map>

#include "interfaces/IAudioManager.h"

class AudioManager : public IAudioManager
{
public:
    AudioManager();
    ~AudioManager();

    // Initialize audio system
    bool Initialize();
    void Shutdown();
    void Update(float deltaTime);

    // Load audio files
    bool LoadSound(const std::string &name, const std::string &filePath) override;
    bool LoadMusic(const std::string &name, const std::string &filePath) override;

    // Play audio
    void PlaySoundEffect(const std::string &name, float volume = 5.0f, float pitch = 1.0f) override;
    void PlayLoopingSoundEffect(const std::string &name, float volume = 1.0f,
                                float pitch = 1.0f) override;
    void StopLoopingSoundEffect(const std::string &name) override;
    void UpdateLoopingSounds() override;
    void PlayMusic(const std::string &name, float volume = 1.0f) override;
    void StopMusic() override;

    // Control music playback
    void PauseMusic() override;
    void ResumeMusic() override;
    bool IsMusicPlaying() const override;

    // Volume control
    void SetMasterVolume(float volume) override;
    void SetMusicVolume(float volume) override;
    void SetSoundVolume(float volume) override;

    float GetMasterVolume() const
    {
        return m_masterVolume;
    }
    float GetMusicVolume() const
    {
        return m_musicVolume;
    }
    float GetSoundVolume() const
    {
        return m_soundVolume;
    }

    // Cleanup
    void UnloadSound(const std::string &name);
    void UnloadMusic(const std::string &name);
    void UnloadAll();

private:
    std::unordered_map<std::string, Sound> m_sounds;
    std::unordered_map<std::string, Music> m_music;
    std::unordered_map<std::string, bool> m_loopingSounds;
    Music m_currentMusic;
    bool m_musicPlaying = false;

    float m_masterVolume = 1.0f;
    float m_musicVolume = 1.0f;
    float m_soundVolume = 1.0f;
    bool m_initialized = false;
};

#endif // AUDIOMANAGER_H

#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <raylib.h>
#include <string>
#include <unordered_map>

#include "interfaces/IAudioManager.h"

class AudioManager : public IAudioManager
{
public:
    static AudioManager &Get()
    {
        static AudioManager instance;
        return instance;
    }

    AudioManager(const AudioManager &) = delete;
    AudioManager &operator=(const AudioManager &) = delete;

    // Initialize audio system
    bool Initialize();
    void Shutdown();
    void Update(float deltaTime);

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
    void SetMasterVolume(float volume);
    void SetMusicVolume(float volume);
    void SetSoundVolume(float volume);

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
    AudioManager();
    ~AudioManager();

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

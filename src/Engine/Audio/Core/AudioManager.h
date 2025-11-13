#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <string>
#include <unordered_map>
#include <memory>
#include <raylib.h>

class AudioManager
{
public:
    AudioManager();
    ~AudioManager();

    // Initialize audio system
    bool Initialize();

    // Load audio files
    bool LoadSound(const std::string& name, const std::string& filePath);
    bool LoadMusic(const std::string& name, const std::string& filePath);

    // Play audio
    void PlaySound(const std::string& name, float volume = 5.0f, float pitch = 1.0f);
    void PlayMusic(const std::string& name, float volume = 1.0f);
    void StopMusic();

    // Control music playback
    void PauseMusic();
    void ResumeMusic();
    bool IsMusicPlaying() const;

    // Volume control
    void SetMasterVolume(float volume);
    void SetMusicVolume(float volume);
    void SetSoundVolume(float volume);

    // Cleanup
    void UnloadSound(const std::string& name);
    void UnloadMusic(const std::string& name);
    void UnloadAll();

private:
    std::unordered_map<std::string, Sound> m_sounds;
    std::unordered_map<std::string, Music> m_music;
    Music m_currentMusic;
    bool m_musicPlaying;

    float m_masterVolume;
    float m_musicVolume;
    float m_soundVolume;
};

#endif // AUDIOMANAGER_H
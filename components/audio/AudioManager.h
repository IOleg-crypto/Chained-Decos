#ifndef SERVERS_AUDIO_MANAGER_H
#define SERVERS_AUDIO_MANAGER_H

#include <raylib.h>
#include <string>
#include <unordered_map>

namespace Servers
{

class AudioManager
{
public:
    AudioManager();
    ~AudioManager();

    // Non-copyable
    AudioManager(const AudioManager &) = delete;
    AudioManager &operator=(const AudioManager &) = delete;

    // Lifecycle
    bool Initialize();
    void Shutdown();

    // Sound management
    bool LoadSound(const std::string &name, const std::string &path);
    void PlaySound(const std::string &name);
    void StopSound(const std::string &name);
    void UnloadSound(const std::string &name);
    void UnloadAllSounds();

    // Music management
    bool LoadMusic(const std::string &name, const std::string &path);
    void PlayMusic(const std::string &name);
    void StopMusic(const std::string &name);
    void UpdateMusic();
    void UnloadMusic(const std::string &name);
    void UnloadAllMusic();

    // Volume control
    void SetMasterVolume(float volume);
    void SetSoundVolume(float volume);
    void SetMusicVolume(float volume);

private:
    std::unordered_map<std::string, Sound> m_sounds;
    std::unordered_map<std::string, Music> m_music;
    float m_master_volume;
    float m_sound_volume;
    float m_music_volume;
    bool m_initialized;
};

} // namespace Servers

#endif // SERVERS_AUDIO_MANAGER_H

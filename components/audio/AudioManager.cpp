#include "AudioManager.h"

namespace Servers
{

AudioManager::AudioManager()
    : m_master_volume(1.0f), m_sound_volume(1.0f), m_music_volume(1.0f), m_initialized(false)
{
}

AudioManager::~AudioManager()
{
    Shutdown();
}

bool AudioManager::Initialize()
{
    if (m_initialized)
        return true;

    InitAudioDevice();
    m_initialized = IsAudioDeviceReady();
    return m_initialized;
}

void AudioManager::Shutdown()
{
    if (!m_initialized)
        return;

    UnloadAllSounds();
    UnloadAllMusic();
    CloseAudioDevice();
    m_initialized = false;
}

bool AudioManager::LoadSound(const std::string &name, const std::string &path)
{
    if (m_sounds.find(name) != m_sounds.end())
        return true;

    Sound sound = ::LoadSound(path.c_str());
    if (sound.frameCount > 0)
    {
        m_sounds[name] = sound;
        return true;
    }
    return false;
}

void AudioManager::PlaySound(const std::string &name)
{
    auto it = m_sounds.find(name);
    if (it != m_sounds.end())
    {
        ::PlaySound(it->second);
    }
}

void AudioManager::StopSound(const std::string &name)
{
    auto it = m_sounds.find(name);
    if (it != m_sounds.end())
    {
        ::StopSound(it->second);
    }
}

void AudioManager::UnloadSound(const std::string &name)
{
    auto it = m_sounds.find(name);
    if (it != m_sounds.end())
    {
        ::UnloadSound(it->second);
        m_sounds.erase(it);
    }
}

void AudioManager::UnloadAllSounds()
{
    for (auto &[name, sound] : m_sounds)
    {
        ::UnloadSound(sound);
    }
    m_sounds.clear();
}

bool AudioManager::LoadMusic(const std::string &name, const std::string &path)
{
    if (m_music.find(name) != m_music.end())
        return true;

    Music music = ::LoadMusicStream(path.c_str());
    if (music.frameCount > 0)
    {
        m_music[name] = music;
        return true;
    }
    return false;
}

void AudioManager::PlayMusic(const std::string &name)
{
    auto it = m_music.find(name);
    if (it != m_music.end())
    {
        ::PlayMusicStream(it->second);
    }
}

void AudioManager::StopMusic(const std::string &name)
{
    auto it = m_music.find(name);
    if (it != m_music.end())
    {
        ::StopMusicStream(it->second);
    }
}

void AudioManager::UpdateMusic()
{
    for (auto &[name, music] : m_music)
    {
        ::UpdateMusicStream(music);
    }
}

void AudioManager::UnloadMusic(const std::string &name)
{
    auto it = m_music.find(name);
    if (it != m_music.end())
    {
        ::UnloadMusicStream(it->second);
        m_music.erase(it);
    }
}

void AudioManager::UnloadAllMusic()
{
    for (auto &[name, music] : m_music)
    {
        ::UnloadMusicStream(music);
    }
    m_music.clear();
}

void AudioManager::SetMasterVolume(float volume)
{
    m_master_volume = volume;
    ::SetMasterVolume(volume);
}

void AudioManager::SetSoundVolume(float volume)
{
    m_sound_volume = volume;
}

void AudioManager::SetMusicVolume(float volume)
{
    m_music_volume = volume;
}

} // namespace Servers

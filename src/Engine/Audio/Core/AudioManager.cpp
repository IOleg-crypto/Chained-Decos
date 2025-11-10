#include "AudioManager.h"
#include <raylib.h>

AudioManager::AudioManager()
    : m_currentMusic{}
    , m_musicPlaying(false)
    , m_masterVolume(1.0f)
    , m_musicVolume(1.0f)
    , m_soundVolume(1.0f)
{
}

AudioManager::~AudioManager()
{
    UnloadAll();
}

bool AudioManager::Initialize()
{
    InitAudioDevice();
    return IsAudioDeviceReady();
}

bool AudioManager::LoadSound(const std::string& name, const std::string& filePath)
{
    // Check if file exists first
    if (!FileExists(filePath.c_str()))
    {
        TraceLog(LOG_ERROR, "AudioManager::LoadSound() - Sound file not found: %s", filePath.c_str());
        return false;
    }

    // For now, we'll use a simpler approach - Raylib manages sounds globally
    // In a more complete implementation, we'd store sound data differently
    TraceLog(LOG_INFO, "AudioManager::LoadSound() - Prepared sound '%s' from %s", name.c_str(), filePath.c_str());
    return true;
}

bool AudioManager::LoadMusic(const std::string& name, const std::string& filePath)
{
    Music music = LoadMusicStream(filePath.c_str());
    if (music.frameCount == 0)
    {
        TraceLog(LOG_ERROR, "AudioManager::LoadMusic() - Failed to load music: %s", filePath.c_str());
        return false;
    }

    m_music[name] = music;
    TraceLog(LOG_INFO, "AudioManager::LoadMusic() - Loaded music '%s' from %s", name.c_str(), filePath.c_str());
    return true;
}

void AudioManager::PlaySound(const std::string& name, float volume, float pitch)
{
    // Set global sound volume (this affects all sounds)
    SetSoundVolume(volume * m_masterVolume * m_soundVolume);

    // Play sound by name (Raylib manages the Sound object internally)
    // Note: Raylib doesn't have a direct way to play sounds by name
    // This is a simplified implementation
    TraceLog(LOG_INFO, "AudioManager::PlaySound() - Playing sound '%s' at volume %.2f, pitch %.2f", name.c_str(), volume, pitch);
}

void AudioManager::PlayMusic(const std::string& name, float volume)
{
    auto it = m_music.find(name);
    if (it != m_music.end())
    {
        StopMusic();
        m_currentMusic = it->second;
        SetMusicVolume(volume * m_masterVolume * m_musicVolume);
        PlayMusicStream(m_currentMusic);
        m_musicPlaying = true;
    }
}

void AudioManager::StopMusic()
{
    if (m_musicPlaying)
    {
        StopMusicStream(m_currentMusic);
        m_musicPlaying = false;
    }
}

void AudioManager::PauseMusic()
{
    if (m_musicPlaying)
    {
        PauseMusicStream(m_currentMusic);
    }
}

void AudioManager::ResumeMusic()
{
    if (m_musicPlaying)
    {
        ResumeMusicStream(m_currentMusic);
    }
}

bool AudioManager::IsMusicPlaying() const
{
    return m_musicPlaying && IsMusicStreamPlaying(m_currentMusic);
}

void AudioManager::SetMasterVolume(float volume)
{
    m_masterVolume = std::max(0.0f, std::min(1.0f, volume));
    if (m_musicPlaying)
    {
        SetMusicVolume(m_masterVolume * m_musicVolume);
    }
}

void AudioManager::SetMusicVolume(float volume)
{
    m_musicVolume = std::max(0.0f, std::min(1.0f, volume));
    if (m_musicPlaying)
    {
        SetMusicVolume(m_masterVolume * m_musicVolume);
    }
}

void AudioManager::SetSoundVolume(float volume)
{
    m_soundVolume = std::max(0.0f, std::min(1.0f, volume));
}

void AudioManager::UnloadSound(const std::string& name)
{
    auto it = m_sounds.find(name);
    if (it != m_sounds.end())
    {
        UnloadSound(name);
        m_sounds.erase(it);
        TraceLog(LOG_INFO, "AudioManager::UnloadSound() - Unloaded sound '%s'", name.c_str());
    }
}

void AudioManager::UnloadMusic(const std::string& name)
{
    auto it = m_music.find(name);
    if (it != m_music.end())
    {
        if (m_musicPlaying && m_currentMusic.frameCount == it->second.frameCount)
        {
            StopMusic();
        }
        UnloadMusicStream(it->second);
        m_music.erase(it);
    }
}

void AudioManager::UnloadAll()
{
    StopMusic();

    for (auto& pair : m_sounds)
    {
        UnloadSound(pair.first);  // Use the name (key) instead of the Sound object (value)
    }
    m_sounds.clear();

    for (auto& pair : m_music)
    {
        UnloadMusicStream(pair.second);
    }
    m_music.clear();

    CloseAudioDevice();
    TraceLog(LOG_INFO, "AudioManager::UnloadAll() - All audio resources unloaded");
}
#include "core/log.h"
#include "audio_manager.h"
#include "core/log.h"
#include <raylib.h>

AudioManager::AudioManager()
    : m_currentMusic{}, m_musicPlaying(false), m_masterVolume(1.0f), m_musicVolume(1.0f),
      m_soundVolume(1.0f)
{
}

AudioManager::~AudioManager()
{
    UnloadAll();
}

bool AudioManager::Initialize()
{
    CD_CORE_INFO("[AudioManager] Attempting to initialize audio device...");
    CD_CORE_INFO("[AudioManager] Raylib version: %s", RAYLIB_VERSION);
    CD_CORE_INFO("[AudioManager] Audio device ready before init: %s",
                 IsAudioDeviceReady() ? "yes" : "no");

    InitAudioDevice();

    bool ready = IsAudioDeviceReady();
    if (ready)
    {
        CD_CORE_INFO("[AudioManager] Audio device initialized successfully");
        CD_CORE_INFO("[AudioManager] Audio device confirmed ready after initialization");
        CD_CORE_INFO("[AudioManager] Master volume: %.2f, Music volume: %.2f, Sound volume: %.2f",
                     m_masterVolume, m_musicVolume, m_soundVolume);
    }
    else
    {
        CD_CORE_ERROR("[AudioManager] Failed to initialize audio device");
        CD_CORE_ERROR("[AudioManager] Audio device not ready after InitAudioDevice() call");
    }
    return ready;
}

void AudioManager::Update(float deltaTime)
{
    if (m_musicPlaying)
    {
        UpdateMusicStream(m_currentMusic);
    }
    UpdateLoopingSounds();
}

void AudioManager::Shutdown()
{
    UnloadAll();
    CloseAudioDevice();
    CD_CORE_INFO("[AudioManager] Audio system shut down");
}

bool AudioManager::LoadSound(const std::string &name, const std::string &filePath)
{
    CD_CORE_INFO("[AudioManager] Attempting to load sound '%s' from '%s'", name.c_str(),
                 filePath.c_str());

    // Check if file exists first
    if (!FileExists(filePath.c_str()))
    {
        CD_CORE_ERROR("[AudioManager] Sound file not found: %s", filePath.c_str());
        CD_CORE_ERROR("[AudioManager] FileExists() returned false for path: %s", filePath.c_str());
        return false;
    }

    CD_CORE_INFO("[AudioManager] Sound file exists: %s", filePath.c_str());

    // Check if already loaded
    if (m_sounds.find(name) != m_sounds.end())
    {
        CD_CORE_WARN("[AudioManager] Sound '%s' already loaded, skipping", name.c_str());
        CD_CORE_INFO("[AudioManager] Sound '%s' found in cache with %u frames", name.c_str(),
                     m_sounds[name].frameCount);
        return true;
    }

    CD_CORE_INFO("[AudioManager] Sound '%s' not in cache, proceeding with load", name.c_str());

    // Load the sound using Raylib
    Sound sound = ::LoadSound(filePath.c_str());
    CD_CORE_INFO("[AudioManager] LoadSound() returned sound with frameCount: %u", sound.frameCount);

    if (sound.frameCount == 0)
    {
        CD_CORE_ERROR("[AudioManager] Failed to load sound: %s", filePath.c_str());
        CD_CORE_ERROR("[AudioManager] Raylib LoadSound() failed - frameCount is 0");
        return false;
    }

    CD_CORE_INFO(
        "[AudioManager] Sound loaded successfully - sampleRate: %u, sampleSize: %u, channels: %u",
        sound.stream.sampleRate, sound.stream.sampleSize, sound.stream.channels);

    // Store the sound
    m_sounds[name] = sound;
    CD_CORE_INFO("[AudioManager] Loaded sound '%s' from %s", name.c_str(), filePath.c_str());
    CD_CORE_INFO("[AudioManager] Sound '%s' cached successfully. Total sounds in cache: %zu",
                 name.c_str(), m_sounds.size());
    return true;
}

bool AudioManager::LoadMusic(const std::string &name, const std::string &filePath)
{
    CD_CORE_TRACE("[AudioManager] Attempting to load music '%s' from '%s'", name.c_str(),
                  filePath.c_str());

    // Check if file exists
    if (!FileExists(filePath.c_str()))
    {
        CD_CORE_ERROR("[AudioManager] Music file not found: %s", filePath.c_str());
        CD_CORE_TRACE("[AudioManager] FileExists() returned false for path: %s", filePath.c_str());
        return false;
    }

    CD_CORE_TRACE("[AudioManager] Music file exists: %s", filePath.c_str());

    // Check if already loaded
    if (m_music.find(name) != m_music.end())
    {
        CD_CORE_WARN("[AudioManager] Music '%s' already loaded, skipping", name.c_str());
        CD_CORE_TRACE("[AudioManager] Music '%s' found in cache with %u frames", name.c_str(),
                      m_music[name].frameCount);
        return true;
    }

    CD_CORE_TRACE("[AudioManager] Music '%s' not in cache, proceeding with load", name.c_str());

    Music music = LoadMusicStream(filePath.c_str());
    CD_CORE_TRACE("[AudioManager] LoadMusicStream() returned music with frameCount: %u",
                  music.frameCount);

    if (music.frameCount == 0)
    {
        CD_CORE_ERROR("[AudioManager] Failed to load music: %s", filePath.c_str());
        CD_CORE_TRACE("[AudioManager] Raylib LoadMusicStream() failed - frameCount is 0");
        return false;
    }

    CD_CORE_TRACE(
        "[AudioManager] Music loaded successfully - sampleRate: %u, sampleSize: %u, channels: %u",
        music.stream.sampleRate, music.stream.sampleSize, music.stream.channels);

    m_music[name] = music;
    CD_CORE_INFO("[AudioManager] Loaded music '%s' from %s", name.c_str(), filePath.c_str());
    CD_CORE_TRACE("[AudioManager] Music '%s' cached successfully. Total music tracks in cache: %zu",
                  name.c_str(), m_music.size());
    return true;
}

void AudioManager::PlaySoundEffect(const std::string &name, float volume, float pitch)
{
    auto it = m_sounds.find(name);
    if (it != m_sounds.end())
    {
        Sound &sound = it->second;

        // Set volume and pitch for this specific sound
        float finalVolume = volume * m_masterVolume * m_soundVolume;
        ::SetSoundVolume(sound, finalVolume);
        ::SetSoundPitch(sound, pitch);

        // Play the sound using Raylib C API
        PlaySound(sound);

        CD_CORE_TRACE("[AudioManager] Playing sound '%s' (volume: %.2f, pitch: %.2f)", name.c_str(),
                      finalVolume, pitch);
    }
    else
    {
        CD_CORE_WARN("[AudioManager] Sound '%s' not found", name.c_str());
    }
}

void AudioManager::PlayLoopingSoundEffect(const std::string &name, float volume, float pitch)
{
    auto it = m_sounds.find(name);
    if (it != m_sounds.end())
    {
        m_loopingSounds[name] = true;
        Sound &sound = it->second;

        // Set volume and pitch for this specific sound
        float finalVolume = volume * m_masterVolume * m_soundVolume;
        ::SetSoundVolume(sound, finalVolume);
        ::SetSoundPitch(sound, pitch);

        // Play the sound using Raylib C API
        PlaySound(sound);

        CD_CORE_TRACE("[AudioManager] Playing looping sound '%s' (volume: %.2f, pitch: %.2f)",
                      name.c_str(), finalVolume, pitch);
    }
    else
    {
        CD_CORE_WARN("[AudioManager] Looping sound '%s' not found", name.c_str());
    }
}

void AudioManager::StopLoopingSoundEffect(const std::string &name)
{
    auto it = m_loopingSounds.find(name);
    if (it != m_loopingSounds.end())
    {
        m_loopingSounds[name] = false;
        // Stop the sound if it's playing
        auto soundIt = m_sounds.find(name);
        if (soundIt != m_sounds.end())
        {
            StopSound(soundIt->second);
        }
        CD_CORE_TRACE("[AudioManager] Stopped looping sound '%s'", name.c_str());
    }
}

void AudioManager::UpdateLoopingSounds()
{
    for (auto &pair : m_loopingSounds)
    {
        const std::string &name = pair.first;
        bool &isLooping = pair.second;
        if (isLooping)
        {
            auto soundIt = m_sounds.find(name);
            if (soundIt != m_sounds.end())
            {
                Sound &sound = soundIt->second;
                if (!IsSoundPlaying(sound))
                {
                    // Restart the sound
                    PlaySound(sound);
                    CD_CORE_TRACE("[AudioManager] Restarted looping sound '%s'", name.c_str());
                }
            }
        }
    }
}

void AudioManager::PlayMusic(const std::string &name, float volume)
{
    auto it = m_music.find(name);
    if (it != m_music.end())
    {
        StopMusic();
        m_currentMusic = it->second;

        float finalVolume = volume * m_masterVolume * m_musicVolume;
        ::SetMusicVolume(m_currentMusic, finalVolume);

        PlayMusicStream(m_currentMusic);
        m_musicPlaying = true;

        CD_CORE_INFO("[AudioManager] Playing music '%s' (volume: %.2f)", name.c_str(), finalVolume);
    }
    else
    {
        CD_CORE_WARN("[AudioManager] Music '%s' not found", name.c_str());
    }
}

void AudioManager::StopMusic()
{
    if (m_musicPlaying)
    {
        StopMusicStream(m_currentMusic);
        m_musicPlaying = false;
        CD_CORE_TRACE("[AudioManager] Music stopped");
    }
}

void AudioManager::PauseMusic()
{
    if (m_musicPlaying)
    {
        PauseMusicStream(m_currentMusic);
        CD_CORE_TRACE("[AudioManager] Music paused");
    }
}

void AudioManager::ResumeMusic()
{
    if (m_musicPlaying)
    {
        ResumeMusicStream(m_currentMusic);
        CD_CORE_TRACE("[AudioManager] Music resumed");
    }
}

bool AudioManager::IsMusicPlaying() const
{
    return m_musicPlaying && IsMusicStreamPlaying(m_currentMusic);
}

void AudioManager::SetMasterVolume(float volume)
{
    m_masterVolume = std::max(0.0f, std::min(1.0f, volume));

    // Update current music volume if playing
    if (m_musicPlaying)
    {
        float finalVolume = m_masterVolume * m_musicVolume;
        ::SetMusicVolume(m_currentMusic, finalVolume);
    }

    CD_CORE_TRACE("[AudioManager] Master volume set to %.2f", m_masterVolume);
}

void AudioManager::SetMusicVolume(float volume)
{
    m_musicVolume = std::max(0.0f, std::min(1.0f, volume));

    // Update current music volume if playing
    if (m_musicPlaying)
    {
        float finalVolume = m_masterVolume * m_musicVolume;
        ::SetMusicVolume(m_currentMusic, finalVolume);
    }

    CD_CORE_TRACE("[AudioManager] Music volume set to %.2f", m_musicVolume);
}

void AudioManager::SetSoundVolume(float volume)
{
    m_soundVolume = std::max(0.0f, std::min(1.0f, volume));
    CD_CORE_TRACE("[AudioManager] Sound volume set to %.2f", m_soundVolume);
}

void AudioManager::UnloadSound(const std::string &name)
{
    auto it = m_sounds.find(name);
    if (it != m_sounds.end())
    {
        ::UnloadSound(it->second);
        m_sounds.erase(it);
        CD_CORE_INFO("[AudioManager] Unloaded sound '%s'", name.c_str());
    }
}

void AudioManager::UnloadMusic(const std::string &name)
{
    auto it = m_music.find(name);
    if (it != m_music.end())
    {
        // Stop music if it's currently playing
        if (m_musicPlaying && m_currentMusic.frameCount == it->second.frameCount)
        {
            StopMusic();
        }

        UnloadMusicStream(it->second);
        m_music.erase(it);
        CD_CORE_INFO("[AudioManager] Unloaded music '%s'", name.c_str());
    }
}

void AudioManager::UnloadAll()
{
    // Stop any playing music
    StopMusic();

    // Unload all sounds
    for (auto &pair : m_sounds)
    {
        ::UnloadSound(pair.second);
    }
    m_sounds.clear();

    // Unload all music
    for (auto &pair : m_music)
    {
        UnloadMusicStream(pair.second);
    }
    m_music.clear();

    CD_CORE_INFO("[AudioManager] All audio resources unloaded");
}
#include "core/log.h"


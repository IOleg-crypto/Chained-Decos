#include "audio_manager.h"
#include "engine/core/log.h"

namespace CHEngine
{
std::unordered_map<std::string, Sound> AudioManager::s_Sounds;

void AudioManager::Init()
{
    InitAudioDevice();
    if (IsAudioDeviceReady())
    {
        CH_CORE_INFO("Audio Device Initialized Successfully");
    }
    else
    {
        CH_CORE_ERROR("Failed to initialize Audio Device!");
    }
}

void AudioManager::Shutdown()
{
    for (auto &[name, sound] : s_Sounds)
    {
        UnloadSound(sound);
    }
    s_Sounds.clear();
    CloseAudioDevice();
    CH_CORE_INFO("Audio Device Shutdown.");
}

void AudioManager::LoadSound(const std::string &name, const std::string &path)
{
    if (s_Sounds.find(name) != s_Sounds.end())
        return;

    Sound sound = ::LoadSound(path.c_str());
    if (sound.frameCount > 0)
    {
        s_Sounds[name] = sound;
        CH_CORE_INFO("Loaded sound: {0} from {1}", name, path);
    }
    else
    {
        CH_CORE_ERROR("Failed to load sound: {0} from {1}", name, path);
    }
}

void AudioManager::PlaySound(const std::string &name, float volume, float pitch)
{
    if (s_Sounds.find(name) == s_Sounds.end())
    {
        CH_CORE_WARN("Attempted to play unknown sound: {0}", name);
        return;
    }

    Sound sound = s_Sounds[name];
    SetSoundVolume(sound, volume);
    SetSoundPitch(sound, pitch);
    ::PlaySound(sound);
}

void AudioManager::StopSound(const std::string &name)
{
    if (s_Sounds.find(name) != s_Sounds.end())
    {
        ::StopSound(s_Sounds[name]);
    }
}

void AudioManager::SetMasterVolume(float volume)
{
    ::SetMasterVolume(volume);
}
} // namespace CHEngine

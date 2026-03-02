#include "audio.h"
#include "engine/core/application.h"
#include "engine/core/log.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"

namespace CHEngine
{
Audio::Audio()
{
}

Audio::~Audio()
{
    // Caution: Shutdown might have been called already
}

Audio* Audio::s_Instance = nullptr;

Audio& Audio::Get()
{
    CH_CORE_ASSERT(s_Instance, "Audio is not initialized!");
    return *s_Instance;
}

void Audio::Init()
{
    if (s_Instance)
    {
        CH_CORE_WARN("Audio is already initialized!");
        return;
    }

    s_Instance = new Audio();

    if (!IsAudioDeviceReady())
    {
        InitAudioDevice();
        CH_CORE_INFO("Audio System Initialized.");
    }
}

void Audio::Shutdown()
{
    if (!s_Instance)
    {
        return;
    }

    // Unload all cached sounds
    for (auto& [path, sound] : s_Instance->m_Sounds)
    {
        if (sound.stream.buffer != nullptr)
        {
            UnloadSound(sound);
        }
    }
    s_Instance->m_Sounds.clear();

    if (IsAudioDeviceReady())
    {
        CloseAudioDevice();
        CH_CORE_INFO("Audio System Shutdown.");
    }

    Audio* instance = s_Instance;
    s_Instance = nullptr;
    delete instance;
}

void Audio::Update(Scene* scene, Timestep ts)
{
    auto& registry = scene->GetRegistry();
    auto view = registry.view<AudioComponent>();

    for (auto entity : view)
    {
        auto& audio = view.get<AudioComponent>(entity);
        if (audio.SoundPath.empty())
        {
            continue;
        }

        Sound sound = GetOrLoadSound(audio.SoundPath);
        if (sound.stream.buffer != nullptr)
        {
            SetSoundVolume(sound, audio.Volume);
            SetSoundPitch(sound, audio.Pitch);

            if (audio.Loop && !IsSoundPlaying(sound) && audio.IsPlaying)
            {
                PlaySound(sound);
            }
        }
    }
}

void Audio::Play(const std::string& path, float volume, float pitch, bool loop)
{
    Sound sound = GetOrLoadSound(path);
    if (sound.stream.buffer != nullptr)
    {
        SetSoundVolume(sound, volume);
        SetSoundPitch(sound, pitch);
        PlaySound(sound);
    }
}

void Audio::Stop(const std::string& path)
{
    auto it = m_Sounds.find(path);
    if (it != m_Sounds.end())
    {
        StopSound(it->second);
    }
}

Sound Audio::GetOrLoadSound(const std::string& path)
{
    auto it = m_Sounds.find(path);
    if (it != m_Sounds.end())
    {
        return it->second;
    }

    if (path.empty())
    {
        return {0};
    }

    Sound sound = LoadSound(path.c_str());
    if (sound.stream.buffer != nullptr)
    {
        m_Sounds[path] = sound;
        CH_CORE_INFO("Audio: Loaded sound from '{}'", path);
    }
    else
    {
        CH_CORE_ERROR("Audio: Failed to load sound from '{}'", path);
    }

    return sound;
}

} // namespace CHEngine

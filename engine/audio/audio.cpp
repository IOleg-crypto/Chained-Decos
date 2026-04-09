#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "audio.h"
#include "engine/core/log.h"

namespace CHEngine
{
static std::vector<std::shared_ptr<SoundInstance>> s_ActiveSounds;
static std::mutex s_AudioMutex;

Audio::Audio()
{
    m_Engine = new ma_engine();
    ma_result result = ma_engine_init(NULL, (ma_engine*)m_Engine);
    if (result != MA_SUCCESS)
    {
        delete (ma_engine*)m_Engine;
        m_Engine = nullptr;
        CH_CORE_ERROR("Audio System: Failed to initialize ma_engine!");
        return;
    }

    CH_CORE_INFO("Audio System: High-level ma_engine initialized.");
}

Audio::~Audio()
{
    if (m_Engine)
    {
        StopAll();
        ma_engine_uninit((ma_engine*)m_Engine);
        delete (ma_engine*)m_Engine;
        m_Engine = nullptr;
        CH_CORE_INFO("Audio System: Shutdown.");
    }
}

void Audio::Update(Timestep ts)
{
    if (!m_Engine) return;

    std::lock_guard<std::mutex> lock(s_AudioMutex);
    for (auto it = s_ActiveSounds.begin(); it != s_ActiveSounds.end(); )
    {
        if (ma_sound_at_end(&(*it)->Sound))
        {
            ma_sound_uninit(&(*it)->Sound);
            ma_audio_buffer_uninit(&(*it)->Buffer);
            it = s_ActiveSounds.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Audio::SetListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up)
{
    if (!m_Engine) return;

    ma_engine_listener_set_position((ma_engine*)m_Engine, 0, position.x, position.y, position.z);
    ma_engine_listener_set_direction((ma_engine*)m_Engine, 0, forward.x, forward.y, forward.z);
    ma_engine_listener_set_world_up((ma_engine*)m_Engine, 0, up.x, up.y, up.z);
}

void Audio::Play(const AudioBuffer& buffer, float volume, float pitch, bool loop, bool spatial, const glm::vec3& pos)
{
    if (!m_Engine) return;

    if (!buffer.Data || buffer.Size == 0 || buffer.Channels == 0 || buffer.SampleRate == 0)
    {
        CH_CORE_WARN("Audio System: Invalid buffer passed to Play() (e.g. 0 channels/size)");
        return;
    }

    auto instance = std::make_shared<SoundInstance>();
    
    ma_uint32 frameCount = buffer.Size / buffer.Channels;
    ma_audio_buffer_config config = ma_audio_buffer_config_init(ma_format_f32, buffer.Channels, frameCount, buffer.Data, NULL);
    config.sampleRate = buffer.SampleRate; // Fix: Assign actual sample rate
    
    ma_result result = ma_audio_buffer_init(&config, &instance->Buffer);
    if (result != MA_SUCCESS)
    {
        CH_CORE_ERROR("Audio System: Failed to initialize audio buffer.");
        return;
    }

    result = ma_sound_init_from_data_source((ma_engine*)m_Engine, &instance->Buffer, 0, NULL, &instance->Sound);
    if (result != MA_SUCCESS)
    {
        ma_audio_buffer_uninit(&instance->Buffer);
        CH_CORE_ERROR("Audio System: Failed to init sound from data source.");
        return;
    }

    ma_sound_set_volume(&instance->Sound, volume);
    ma_sound_set_pitch(&instance->Sound, pitch);
    ma_sound_set_looping(&instance->Sound, loop);
    
    if (spatial)
    {
        ma_sound_set_position(&instance->Sound, pos.x, pos.y, pos.z);
        ma_sound_set_spatialization_enabled(&instance->Sound, MA_TRUE);
    }

    result = ma_sound_start(&instance->Sound);
    if (result != MA_SUCCESS)
    {
        ma_sound_uninit(&instance->Sound);
        ma_audio_buffer_uninit(&instance->Buffer);
        CH_CORE_ERROR("Audio System: Failed to start sound.");
        return;
    }

    std::lock_guard<std::mutex> lock(s_AudioMutex);
    s_ActiveSounds.push_back(instance);
}

void Audio::StopAll()
{
    if (!m_Engine) return;

    std::lock_guard<std::mutex> lock(s_AudioMutex);
    for (auto& instance : s_ActiveSounds)
    {
        ma_sound_stop(&instance->Sound);
        ma_sound_uninit(&instance->Sound);
        ma_audio_buffer_uninit(&instance->Buffer);
    }
    s_ActiveSounds.clear();
}

} // namespace CHEngine

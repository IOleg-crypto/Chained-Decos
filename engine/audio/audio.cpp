#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "audio.h"
#include "engine/core/service_locator.h"
#include "engine/core/log.h"
#include "engine/scene/project.h"
#include <filesystem>

namespace CHEngine
{

Audio& Audio::Get()
{
    return ServiceLocator::Get<Audio>();
}


static std::vector<std::shared_ptr<SoundInstance>> s_ActiveSounds;

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

    std::lock_guard<std::mutex> lock(m_DataMutex);
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

AudioHandle Audio::LoadSound(const std::string& filepath)
{
    if (filepath.empty())
    {
        return 0;
    }

    std::filesystem::path resolvedPath = Project::GetAbsolutePath(filepath);
    if (resolvedPath.empty() || !std::filesystem::exists(resolvedPath))
    {
        CH_CORE_ERROR("Audio System: File not found: {}", filepath);
        return 0; // Returning 0 (INVALID_HANDLE equivalent for UUIDs typically)
    }

    std::string cacheKey = resolvedPath.generic_string();

    {
        std::lock_guard<std::mutex> lock(m_DataMutex);
        auto existing = m_PathRegistry.find(cacheKey);
        if (existing != m_PathRegistry.end())
        {
            return existing->second;
        }
    }

    ma_decoder decoder;
    ma_result result = ma_decoder_init_file(cacheKey.c_str(), NULL, &decoder);
    if (result != MA_SUCCESS)
    {
        CH_CORE_ERROR("Audio System: Failed to initialize decoder for {}", filepath);
        return 0;
    }

    ma_uint64 frameCount;
    result = ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);
    if (result != MA_SUCCESS)
    {
        CH_CORE_ERROR("Audio System: Failed to get length for {}", filepath);
        ma_decoder_uninit(&decoder);
        return 0;
    }

    std::vector<float> pcmData(frameCount * decoder.outputChannels);
    ma_uint64 framesRead;
    result = ma_decoder_read_pcm_frames(&decoder, pcmData.data(), frameCount, &framesRead);
    
    uint32_t channels = decoder.outputChannels;
    uint32_t sampleRate = decoder.outputSampleRate;

    ma_decoder_uninit(&decoder);

    if (result != MA_SUCCESS)
    {
        CH_CORE_ERROR("Audio System: Failed to read PCM frames for {}", filepath);
        return 0;
    }

    std::lock_guard<std::mutex> lock(m_DataMutex);
    AudioHandle newHandle = UUID(); // Generate new unique ID
    
    AudioData data;
    data.PCMData = std::move(pcmData);
    data.Channels = channels;
    data.SampleRate = sampleRate;

    m_AudioDataRegistry[newHandle] = std::move(data);
    m_PathRegistry[cacheKey] = newHandle;
    
    CH_CORE_INFO("Audio System: Successfully loaded {} ({} frames)", filepath, frameCount);
    return newHandle;
}

bool Audio::IsSoundLoaded(AudioHandle handle) const
{
    std::lock_guard<std::mutex> lock(m_DataMutex);
    return m_AudioDataRegistry.find(handle) != m_AudioDataRegistry.end();
}

bool Audio::IsPlaying(AudioHandle handle) const
{
    if (handle == 0)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_DataMutex);
    for (const auto& instance : s_ActiveSounds)
    {
        if (instance && instance->Handle == handle)
        {
            return true;
        }
    }
    return false;
}

void Audio::SetListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up)
{
    if (!m_Engine) return;

    ma_engine_listener_set_position((ma_engine*)m_Engine, 0, position.x, position.y, position.z);
    ma_engine_listener_set_direction((ma_engine*)m_Engine, 0, forward.x, forward.y, forward.z);
    ma_engine_listener_set_world_up((ma_engine*)m_Engine, 0, up.x, up.y, up.z);
}

void Audio::Play(AudioHandle handle, float volume, float pitch, bool loop, bool spatial, const glm::vec3& pos)
{
    if (!m_Engine || handle == 0) return;

    std::lock_guard<std::mutex> lock(m_DataMutex);
    auto it = m_AudioDataRegistry.find(handle);
    if (it == m_AudioDataRegistry.end()) return;

    const AudioData& data = it->second;

    if (data.PCMData.empty() || data.Channels == 0 || data.SampleRate == 0)
    {
        CH_CORE_WARN("Audio System: Invalid data in registry for Handle {}", (uint64_t)handle);
        return;
    }

    auto instance = std::make_shared<SoundInstance>();
    instance->Handle = handle;
    
    ma_uint32 frameCount = data.Size() / data.Channels;
    ma_audio_buffer_config config = ma_audio_buffer_config_init(ma_format_f32, data.Channels, frameCount, data.Data(), NULL);
    config.sampleRate = data.SampleRate; 
    
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

    s_ActiveSounds.push_back(instance);
}

void Audio::Stop(const std::string& filepath)
{
    if (!m_Engine || filepath.empty()) return;

    std::filesystem::path resolvedPath = Project::GetAbsolutePath(filepath);
    if (resolvedPath.empty()) return;

    AudioHandle handle = 0;
    {
        std::lock_guard<std::mutex> lock(m_DataMutex);
        auto it = m_PathRegistry.find(resolvedPath.generic_string());
        if (it == m_PathRegistry.end())
        {
            return;
        }
        handle = it->second;
    }

    Stop(handle);
}

void Audio::Stop(AudioHandle handle)
{
    if (!m_Engine || handle == 0) return;

    std::lock_guard<std::mutex> lock(m_DataMutex);
    for (auto it = s_ActiveSounds.begin(); it != s_ActiveSounds.end(); )
    {
        if ((*it)->Handle == handle)
        {
            ma_sound_stop(&(*it)->Sound);
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

void Audio::StopAll()
{
    if (!m_Engine) return;

    std::lock_guard<std::mutex> lock(m_DataMutex);
    for (auto& instance : s_ActiveSounds)
    {
        ma_sound_stop(&instance->Sound);
        ma_sound_uninit(&instance->Sound);
        ma_audio_buffer_uninit(&instance->Buffer);
    }
    s_ActiveSounds.clear();
}

} // namespace CHEngine

#include "audio.h"
#include "engine/core/application.h"
#include "engine/core/log.h"

#include "miniaudio.h"

namespace CHEngine
{

static Audio* s_Instance = nullptr;

Audio::Audio()
{
    CH_CORE_ASSERT(!s_Instance, "Audio system already exists!");
    s_Instance = this;
    m_Device = new ma_device();
}

Audio::~Audio()
{
    Shutdown();
    delete (ma_device*)m_Device;
    s_Instance = nullptr;
}

Audio& Audio::Get()
{
    CH_CORE_ASSERT(s_Instance, "Audio system not initialized!");
    return *s_Instance;
}

void Audio::Init()
{
    if (!s_Instance)
        s_Instance = new Audio();
    
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate        = 48000;
    config.dataCallback      = (ma_device_data_proc)DataCallback;
    config.pUserData         = s_Instance;

    if (ma_device_init(NULL, &config, (ma_device*)s_Instance->m_Device) != MA_SUCCESS)
    {
        CH_CORE_ERROR("Audio System: Failed to initialize ma_device!");
        return;
    }

    ma_device_start((ma_device*)s_Instance->m_Device);
    CH_CORE_INFO("Audio System Initialized (Low-Level ma_device).");
}

void Audio::Shutdown()
{
    if (s_Instance && s_Instance->m_Device)
    {
        s_Instance->StopAll();
        ma_device_uninit((ma_device*)s_Instance->m_Device);
    }
    CH_CORE_INFO("Audio System Shutdown.");
}

void Audio::Update(Timestep ts)
{
    // Cleanup finished sounds
    std::lock_guard<std::mutex> lock(m_AudioMutex);
    for (auto it = m_ActiveSounds.begin(); it != m_ActiveSounds.end(); )
    {
        if ((*it)->Finished)
        {
            if ((*it)->IsInitialized && (*it)->Decoder)
            {
                ma_decoder_uninit((ma_decoder*)(*it)->Decoder);
                delete (ma_decoder*)(*it)->Decoder;
            }
            it = m_ActiveSounds.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Audio::SetListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up)
{
    std::lock_guard<std::mutex> lock(m_AudioMutex);
    m_Listener.Position = position;
    m_Listener.Forward = forward;
    m_Listener.Up = up;
}

void Audio::Play(const std::string& filepath, float volume, float pitch, bool loop, bool spatial, const glm::vec3& pos)
{
    if (filepath.empty())
        return;

    auto instance = std::make_shared<SoundInstance>();
    instance->Path = filepath;
    instance->Volume = volume;
    instance->Pitch = pitch;
    instance->Loop = loop;
    instance->Spatial = spatial;
    instance->Position = pos;

    instance->Decoder = new ma_decoder();
    ma_result result = ma_decoder_init_file(filepath.c_str(), NULL, (ma_decoder*)instance->Decoder);
    if (result == MA_SUCCESS)
    {
        instance->IsInitialized = true;
        std::lock_guard<std::mutex> lock(m_AudioMutex);
        m_ActiveSounds.push_back(instance);
    }
    else
    {
        CH_CORE_ERROR("Audio System: Failed to init decoder for {}", filepath);
        delete (ma_decoder*)instance->Decoder;
    }
}

void Audio::Stop(const std::string& filepath)
{
    std::lock_guard<std::mutex> lock(m_AudioMutex);
    for (auto& instance : m_ActiveSounds)
    {
        if (instance->Path == filepath)
            instance->Finished = true;
    }
}

void Audio::StopAll()
{
    std::lock_guard<std::mutex> lock(m_AudioMutex);
    for (auto& sound : m_ActiveSounds)
    {
        if (sound->IsInitialized && sound->Decoder)
        {
            ma_decoder_uninit((ma_decoder*)sound->Decoder);
            delete (ma_decoder*)sound->Decoder;
            sound->Decoder = nullptr;
            sound->IsInitialized = false;
        }
    }
    m_ActiveSounds.clear();
}

void Audio::DataCallback(void* pDevice, void* pOutput, const void* pInput, unsigned int frameCount)
{
    Audio* self = (Audio*)((ma_device*)pDevice)->pUserData;
    if (!self) return;
    
    float* fOutput = (float*)pOutput;

    // Clear buffer
    memset(pOutput, 0, frameCount * 2 * sizeof(float));

    std::lock_guard<std::mutex> lock(self->m_AudioMutex);
    
    float tempBuffer[1024 * 2]; // Stereo
    
    glm::vec3 listenerRight = glm::normalize(glm::cross(self->m_Listener.Forward, self->m_Listener.Up));

    for (auto& sound : self->m_ActiveSounds)
    {
        if (sound->Finished) continue;

        ma_uint32 totalFramesRead = 0;
        while (totalFramesRead < frameCount)
        {
            ma_uint32 framesToRead = frameCount - totalFramesRead;
            if (framesToRead > 1024) framesToRead = 1024;

            ma_uint64 framesRead = 0;
            ma_decoder_read_pcm_frames((ma_decoder*)sound->Decoder, tempBuffer, (ma_uint64)framesToRead, &framesRead);
            
            if (framesRead == 0)
            {
                if (sound->Loop)
                {
                    ma_decoder_seek_to_pcm_frame((ma_decoder*)sound->Decoder, 0);
                    continue;
                }
                else
                {
                    sound->Finished = true;
                    break;
                }
            }

            // Mix into output
            float spatialVolumeLeft = sound->Volume;
            float spatialVolumeRight = sound->Volume;

            if (sound->Spatial)
            {
                glm::vec3 toSound = sound->Position - self->m_Listener.Position;
                float distance = glm::length(toSound);
                
                // Simple distance attenuation
                float attenuation = 1.0f / (1.0f + 0.1f * distance); 
                
                // Simple panning
                if (distance > 0.001f)
                {
                    toSound = glm::normalize(toSound);
                    float pan = glm::dot(toSound, listenerRight); // -1 to 1
                    spatialVolumeLeft *= (1.0f - pan) * 0.5f;
                    spatialVolumeRight *= (1.0f + pan) * 0.5f;
                }
                
                spatialVolumeLeft *= attenuation;
                spatialVolumeRight *= attenuation;
            }

            for (ma_uint32 i = 0; i < framesRead; ++i)
            {
                fOutput[(totalFramesRead + i) * 2 + 0] += tempBuffer[i * 2 + 0] * spatialVolumeLeft;
                fOutput[(totalFramesRead + i) * 2 + 1] += tempBuffer[i * 2 + 1] * spatialVolumeRight;
            }

            totalFramesRead += framesRead;
        }
    }
}

} // namespace CHEngine

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "audio.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "engine/core/log.h"
#include "engine/project/project.h"
#include <filesystem>
#include <algorithm>

namespace Chained
{

Audio::Audio()
    : m_engine(nullptr)
{
}

Audio::~Audio()
{
    if (m_engine)
    {
        CH_CORE_WARN("Audio System: Destructor called before explicit Shutdown(). Force shutting down.");
        Shutdown();
    }
}

void Audio::Initialize()
{
    if (m_engine)
    {
        CH_CORE_WARN("Audio System: Already initialized.");
        return;
    }

    m_engine = std::unique_ptr<ma_engine, MiniaudioEngineDeleter>(new ma_engine());
    ma_result result = ma_engine_init(NULL, m_engine.get());
    if (result != MA_SUCCESS)
    {
        CH_CORE_ERROR("Audio System: Failed to initialize miniaudio engine.");
        m_engine.reset();
    }
    else
    {
        CH_CORE_INFO("Audio System: Initialized miniaudio engine successfully via EngineModule.");
    }
}

void Audio::Shutdown()
{
    if (!m_engine)
    {
        return;
    }

    StopAll();

    m_engine.reset();

    CH_CORE_INFO("Audio System: Shutdown complete.");
}

static void UninitInstance(Chained::SoundInstance& instance)
{
    ma_sound_stop(&instance.Sound);
    ma_sound_uninit(&instance.Sound);
    if (instance.HasDecoder)
    {
        ma_decoder_uninit(&instance.Decoder);
        instance.HasDecoder = false;
    }
}

void Audio::Update(Timestep ts)
{
    if (!m_engine)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_DataMutex);
    for (auto it = m_ActiveSounds.begin(); it != m_ActiveSounds.end();)
    {
        if (ma_sound_at_end(&(*it)->Sound))
        {
            UninitInstance(**it);
            it = m_ActiveSounds.erase(it);
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

    auto project = Project::GetActive();
    std::filesystem::path resolvedPath;
    if (project && std::filesystem::path(filepath).is_relative())
    {
        resolvedPath = project->GetAssetDirectoryForProject() / filepath;
    }
    else
    {
        resolvedPath = filepath;
    }

    auto* am = ServiceLocator::TryGet<AssetManager>();
    bool existsOnDisk = !resolvedPath.empty() && std::filesystem::exists(resolvedPath);
    bool existsInPack = am && (am->HasAsset(filepath) || am->HasAsset(resolvedPath.generic_string()));

    if (!existsOnDisk && !existsInPack)
    {
        CH_CORE_ERROR("Audio System: File not found: {}", filepath);
        return 0;
    }

    std::string cacheKey = filepath;

    std::lock_guard<std::mutex> lock(m_DataMutex);
    auto existing = m_PathToHandle.find(cacheKey);
    if (existing != m_PathToHandle.end())
    {
        return existing->second;
    }

    AudioHandle newHandle = UUID();
    m_PathToHandle[cacheKey] = newHandle;
    m_HandleToPath[newHandle] = filepath;

    CH_CORE_INFO("Audio System: Registered sound path {}", cacheKey);
    return newHandle;
}

bool Audio::IsSoundLoaded(AudioHandle handle) const
{
    std::lock_guard lock(m_DataMutex);
    return m_HandleToPath.contains(handle);
}

bool Audio::IsPlaying(AudioHandle handle) const
{
    if (handle == 0)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_DataMutex);
    for (const auto& instance : m_ActiveSounds)
    {
        if (instance && instance->Handle == handle)
        {
            if (ma_sound_is_playing(&instance->Sound))
            {
                return true;
            }
        }
    }
    return false;
}

void Audio::SetListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up)
{
    if (!m_engine)
    {
        return;
    }

    ma_engine_listener_set_position(m_engine.get(), 0, position.x, position.y, position.z);
    ma_engine_listener_set_direction(m_engine.get(), 0, forward.x, forward.y, forward.z);
    ma_engine_listener_set_world_up(m_engine.get(), 0, up.x, up.y, up.z);
}

void Audio::SetInstancePosition(AudioHandle handle, const glm::vec3& pos)
{
    if (!m_engine || handle == 0)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_DataMutex);
    for (const auto& instance : m_ActiveSounds)
    {
        if (instance && instance->Handle == handle)
        {
            ma_sound_set_position(&instance->Sound, pos.x, pos.y, pos.z);
        }
    }
}

void Audio::Play(AudioHandle handle, float volume, float pitch, bool loop, bool spatial, const glm::vec3& pos)
{
    if (!m_engine || handle == 0)
    {
        return;
    }

    std::string filepath;
    {
        std::lock_guard<std::mutex> lock(m_DataMutex);
        auto it = m_HandleToPath.find(handle);
        if (it == m_HandleToPath.end())
        {
            CH_CORE_WARN("Audio System: Try to play unknown handle {}", (uint64_t)handle);
            return;
        }
        filepath = it->second;
    }

    auto instance = std::make_unique<SoundInstance>();
    instance->Handle = handle;

    ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC;
    ma_result result = MA_ERROR;

    auto* am = ServiceLocator::TryGet<AssetManager>();
    if (am)
    {
        instance->SoundData = am->ReadProjectAsset(filepath);
        if (instance->SoundData.empty())
        {
            instance->SoundData = am->ReadAssetData(filepath);
        }
    }

    if (!instance->SoundData.empty())
    {
        ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
        result = ma_decoder_init_memory(instance->SoundData.data(), instance->SoundData.size(), &config, &instance->Decoder);
        if (result == MA_SUCCESS)
        {
            instance->HasDecoder = true;
            result = ma_sound_init_from_data_source(m_engine.get(), &instance->Decoder, flags, NULL, &instance->Sound);
        }
    }

    if (result != MA_SUCCESS)
    {
        std::filesystem::path resolvedPath = filepath;
        if (auto project = Project::GetActive())
        {
            if (resolvedPath.is_relative())
            {
                resolvedPath = project->GetAssetDirectoryForProject() / filepath;
            }
        }
        result = ma_sound_init_from_file(m_engine.get(), resolvedPath.string().c_str(), flags, NULL, NULL, &instance->Sound);
    }

    if (result != MA_SUCCESS)
    {
        CH_CORE_ERROR("Audio System: Failed to init sound {}", filepath);
        return;
    }

    ma_sound_set_volume(&instance->Sound, volume);
    ma_sound_set_pitch(&instance->Sound, pitch);
    ma_sound_set_looping(&instance->Sound, loop ? MA_TRUE : MA_FALSE);

    if (spatial)
    {
        ma_sound_set_position(&instance->Sound, pos.x, pos.y, pos.z);
        ma_sound_set_spatialization_enabled(&instance->Sound, MA_TRUE);
    }

    result = ma_sound_start(&instance->Sound);
    if (result != MA_SUCCESS)
    {
        UninitInstance(*instance);
        CH_CORE_ERROR("Audio System: Failed to start sound.");
        return;
    }

    std::lock_guard lock(m_DataMutex);
    m_ActiveSounds.push_back(std::move(instance));
}

void Audio::SetVolume(AudioHandle handle, float volume)
{
    if (!m_engine || handle == 0)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_DataMutex);
    for (const auto& instance : m_ActiveSounds)
    {
        if (instance && instance->Handle == handle)
        {
            ma_sound_set_volume(&instance->Sound, volume);
        }
    }
}

void Audio::SetPitch(AudioHandle handle, float pitch)
{
    if (!m_engine || handle == 0)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_DataMutex);
    for (const auto& instance : m_ActiveSounds)
    {
        if (instance && instance->Handle == handle)
        {
            ma_sound_set_pitch(&instance->Sound, pitch);
        }
    }
}

void Audio::Stop(const std::string& filepath)
{
    if (!m_engine || filepath.empty())
    {
        return;
    }

    AudioHandle handle = 0;
    {
        std::lock_guard<std::mutex> lock(m_DataMutex);
        auto it = m_PathToHandle.find(filepath);
        if (it == m_PathToHandle.end())
        {
            return;
        }
        handle = it->second;
    }

    Stop(handle);
}

void Audio::Stop(AudioHandle handle)
{
    if (!m_engine || handle == 0)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_DataMutex);
    for (auto it = m_ActiveSounds.begin(); it != m_ActiveSounds.end();)
    {
        if ((*it)->Handle == handle)
        {
            UninitInstance(**it);
            it = m_ActiveSounds.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Audio::StopAll()
{
    if (!m_engine)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_DataMutex);
    for (auto& instance : m_ActiveSounds)
    {
        UninitInstance(*instance);
    }
    m_ActiveSounds.clear();
}

ma_engine* Audio::GetEngine() const
{
    return m_engine.get();
}

} // namespace Chained
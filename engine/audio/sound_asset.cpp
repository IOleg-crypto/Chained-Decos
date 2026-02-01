#include "sound_asset.h"
#include "engine/core/log.h"
#include "raylib.h"
#include <filesystem>


namespace CHEngine
{
std::shared_ptr<SoundAsset> SoundAsset::Load(const std::string &path)
{
    if (path.empty())
        return nullptr;

    std::filesystem::path fullPath(path);
    if (!std::filesystem::exists(fullPath))
    {
        CH_CORE_ERROR("Sound file not found: {}", path);
        return nullptr;
    }

    auto asset = std::make_shared<SoundAsset>();
    asset->SetPath(path);
    asset->SetState(AssetState::Ready);
    asset->m_Sound = LoadSound(fullPath.string().c_str());

    if (asset->m_Sound.stream.buffer == nullptr)
    {
        CH_CORE_ERROR("Failed to load sound: {}", path);
        return nullptr;
    }

    return asset;
}

void SoundAsset::LoadFromFile(const std::string &path)
{
    if (m_State == AssetState::Ready) return;

    std::filesystem::path fullPath(path);
    if (!std::filesystem::exists(fullPath))
    {
        SetState(AssetState::Failed);
        return;
    }

    // This runs on background thread (std::async)
    m_PendingWave = ::LoadWave(path.c_str());
    
    if (m_PendingWave.frameCount > 0)
        m_HasPendingWave = true;
    else
        SetState(AssetState::Failed);
}

void SoundAsset::UploadToGPU()
{
    // This MUST run on main thread (where Audio device is initialized)
    if (m_HasPendingWave && m_PendingWave.frameCount > 0)
    {
        if (m_Sound.stream.buffer != nullptr)
            ::UnloadSound(m_Sound);
            
        m_Sound = ::LoadSoundFromWave(m_PendingWave);
        ::UnloadWave(m_PendingWave);
        
        m_PendingWave.frameCount = 0;
        m_HasPendingWave = false;
        
        SetState(AssetState::Ready);
    }
}

SoundAsset::~SoundAsset()
{
    if (m_Sound.stream.buffer != nullptr)
    {
        ::UnloadSound(m_Sound);
    }
}

} // namespace CHEngine

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

SoundAsset::~SoundAsset()
{
    if (m_Sound.stream.buffer != nullptr)
    {
        UnloadSound(m_Sound);
    }
}
} // namespace CHEngine

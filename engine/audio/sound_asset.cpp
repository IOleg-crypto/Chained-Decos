#include "sound_asset.h"
#include "engine/core/log.h"
#include "engine/render/asset_manager.h"

namespace CHEngine
{
SoundAsset::SoundAsset(Sound sound) : m_Sound(sound)
{
}

SoundAsset::~SoundAsset()
{
    if (m_Sound.frameCount > 0)
    {
        UnloadSound(m_Sound);
    }
}

std::shared_ptr<SoundAsset> SoundAsset::Load(const std::string &path)
{
    auto absolutePath = AssetManager::ResolvePath(path);
    Sound sound = ::LoadSound(absolutePath.string().c_str());

    if (sound.frameCount > 0)
    {
        auto asset = std::make_shared<SoundAsset>(sound);
        asset->SetPath(path);
        return asset;
    }

    CH_CORE_ERROR("Failed to load sound: {0}", path);
    return nullptr;
}
} // namespace CHEngine

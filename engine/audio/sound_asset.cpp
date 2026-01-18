#include "sound_asset.h"
#include "engine/core/log.h"
#include "engine/renderer/asset_manager.h"


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

Ref<SoundAsset> SoundAsset::Load(const std::string &path)
{
    auto absolutePath = Assets::ResolvePath(path);
    Sound sound = ::LoadSound(absolutePath.string().c_str());

    if (sound.frameCount > 0)
    {
        return CreateRef<SoundAsset>(sound);
    }

    CH_CORE_ERROR("Failed to load sound: {0}", path);
    return nullptr;
}
} // namespace CHEngine

#pragma once
#include "engine/core/base.h"
#include "engine/renderer/asset.h"
#include <raylib.h>
#include <string>

namespace CHEngine
{
class SoundAsset : public Asset
{
public:
    SoundAsset(Sound sound);
    virtual ~SoundAsset();

    static std::shared_ptr<SoundAsset> Load(const std::string &path);

    virtual AssetType GetType() const override
    {
        return AssetType::Audio;
    }

    Sound &GetSound()
    {
        return m_Sound;
    }
    const Sound &GetSound() const
    {
        return m_Sound;
    }

private:
    Sound m_Sound;
};
} // namespace CHEngine

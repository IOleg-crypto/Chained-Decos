#ifndef CH_SOUND_ASSET_H
#define CH_SOUND_ASSET_H
#include "engine/core/base.h"
#include "engine/graphics/asset.h"
#include "raylib.h"
#include <string>

namespace CHEngine
{
class SoundAsset : public Asset
{
public:
    SoundAsset() = default;
    SoundAsset(Sound sound);
    ~SoundAsset() override;

    static std::shared_ptr<SoundAsset> Load(const std::string &path);

    virtual AssetType GetType() const override
    {
        return AssetType::Audio;
    }

    void UploadToGPU();
    void LoadFromFile(const std::string &path);

    Sound &GetSound()
    {
        return m_Sound;
    }
    const Sound &GetSound() const
    {
        return m_Sound;
    }

private:
    Sound m_Sound = { 0 };
    Wave m_PendingWave = {0};
    bool m_HasPendingWave = false;
};
} // namespace CHEngine
#endif // CH_SOUND_ASSET_H

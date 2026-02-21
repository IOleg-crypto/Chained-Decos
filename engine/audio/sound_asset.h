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
    static AssetType GetStaticType()
    {
        return AssetType::Audio;
    }

    SoundAsset()
        : Asset(GetStaticType())
    {
    }
    SoundAsset(Sound sound)
        : Asset(GetStaticType()),
          m_Sound(sound)
    {
    }
    ~SoundAsset() override;

    void UploadToGPU();

    // For internal use by AudioImporter
    void SetPendingWave(Wave wave)
    {
        m_PendingWave = wave;
        m_HasPendingWave = true;
    }

    Sound& GetSound()
    {
        return m_Sound;
    }
    const Sound& GetSound() const
    {
        return m_Sound;
    }

private:
    Sound m_Sound = {0};
    Wave m_PendingWave = {0};
    bool m_HasPendingWave = false;
};
} // namespace CHEngine
#endif // CH_SOUND_ASSET_H

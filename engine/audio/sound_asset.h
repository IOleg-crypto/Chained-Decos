#ifndef CH_SOUND_ASSET_H
#define CH_SOUND_ASSET_H
#include "engine/core/base.h"
#include "engine/core/assets/asset.h"
#include <string>
#include <vector>

namespace CHEngine
{
struct SoundData {
    void* maSound = nullptr; // ma_sound*
};
struct WaveData {
    void* maDecoder = nullptr; // ma_decoder*
};


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
    ~SoundAsset() override;

    void UploadToGPU();

    // For internal use by AudioImporter
    void SetPendingWave(WaveData wave)
    {
        m_PendingWave = wave;
        m_HasPendingWave = true;
    }

    SoundData& GetSound()
    {
        return m_Sound;
    }
    const SoundData& GetSound() const
    {
        return m_Sound;
    }

private:
    SoundData m_Sound;
    WaveData m_PendingWave;
    bool m_HasPendingWave = false;
};
} // namespace CHEngine
#endif // CH_SOUND_ASSET_H

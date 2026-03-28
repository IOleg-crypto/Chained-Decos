#ifndef CH_SOUND_ASSET_H
#define CH_SOUND_ASSET_H
#include "engine/core/assets/asset.h"

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
    ~SoundAsset() override;

private:
};
} // namespace CHEngine
#endif // CH_SOUND_ASSET_H

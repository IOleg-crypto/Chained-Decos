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
};
} // namespace CHEngine
#endif // CH_SOUND_ASSET_H

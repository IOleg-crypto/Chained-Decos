#ifndef CH_ASSET_METADATA_H
#define CH_ASSET_METADATA_H

#include "engine/assets/asset.h"
#include <filesystem>

namespace Chained
{
    struct AssetMetadata
    {
        AssetHandle Handle = 0;
        AssetType Type = AssetType::None;
        std::filesystem::path FilePath;
        bool IsLoaded = false;

        bool IsValid() const { return Handle != 0 && !FilePath.empty(); }
    };
}

#endif // CH_ASSET_METADATA_H

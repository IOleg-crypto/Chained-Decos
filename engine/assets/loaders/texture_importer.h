#ifndef CH_TEXTURE_IMPORTER_H
#define CH_TEXTURE_IMPORTER_H

#include "engine/assets/asset_metadata.h"
#include "engine/assets/types/texture_asset.h"
#include <memory>

namespace Chained
{
class TextureImporter
{
public:
    static std::shared_ptr<TextureAsset> ImportTexture(AssetHandle handle, const AssetMetadata& metadata);
};
} // namespace Chained

#endif // CH_TEXTURE_IMPORTER_H

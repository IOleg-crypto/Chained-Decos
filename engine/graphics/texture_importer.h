#ifndef CH_TEXTURE_IMPORTER_H
#define CH_TEXTURE_IMPORTER_H

#include "asset_importer.h"
#include "texture_asset.h"

namespace CHEngine
{
class TextureImporter : public AssetImporter
{
public:
    static std::shared_ptr<TextureAsset> ImportTexture(const std::filesystem::path& path);

    // For async loading: returns the CPU image data
    static Image LoadImageFromDisk(const std::filesystem::path& path);
};
} // namespace CHEngine

#endif // CH_TEXTURE_IMPORTER_H

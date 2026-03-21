#ifndef CH_TEXTURE_LOADER_H
#define CH_TEXTURE_LOADER_H

#include "engine/core/assets/asset_loader.h"
#include "engine/graphics/assets/texture_asset.h"
#include "engine/graphics/importers/texture_importer.h"
#include "engine/core/log.h"
#include <filesystem>
#include <algorithm>

namespace CHEngine
{
class TextureLoader : public IAssetLoader
{
public:
    std::shared_ptr<Asset> Create() override
    {
        return std::make_shared<TextureAsset>();
    }

    bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath) override
    {
        auto texAsset = std::static_pointer_cast<TextureAsset>(asset);
        
        std::string ext = std::filesystem::path(resolvedPath).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext == ".hdr")
        {
            CH_CORE_INFO("TextureLoader: Recognized {} as HDR file, will load directly on main thread", resolvedPath);
            return true; 
        }

        Image img = TextureImporter::LoadImageFromDisk(resolvedPath);
        if (img.data != nullptr)
        {
            // Note: In raylib/engine context, we often ensure RGBA8
            // ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8); 
            // If ImageFormat is needed, ensure raylib.h or similar is accessible.
            texAsset->SetPendingImage(img);
            return true;
        }
        
        return false;
    }

    bool IsAsync() const override { return true; }
};
} // namespace CHEngine

#endif // CH_TEXTURE_LOADER_H

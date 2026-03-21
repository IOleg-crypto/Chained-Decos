#ifndef CH_FONT_LOADER_H
#define CH_FONT_LOADER_H

#include "engine/core/assets/asset_loader.h"
#include "engine/graphics/assets/font_asset.h"
#include "engine/graphics/importers/font_importer.h"

namespace CHEngine
{
class FontLoader : public IAssetLoader
{
public:
    std::shared_ptr<Asset> Create() override
    {
        return std::make_shared<FontAsset>();
    }

    bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath) override
    {
        auto fontAsset = std::static_pointer_cast<FontAsset>(asset);
        auto imported = FontImporter::ImportFont(resolvedPath);
        if (imported)
        {
            fontAsset->SetFont(imported->GetFont());
            return true;
        }
        return false;
    }

    bool IsAsync() const override { return false; }
};
} // namespace CHEngine

#endif // CH_FONT_LOADER_H

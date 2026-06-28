#ifndef CH_FONT_LOADER_H
#define CH_FONT_LOADER_H

#include "engine/assets/loaders/asset_loader.h"
#include "engine/assets/types/font_asset.h"
#include <memory>
#include <string>

namespace Chained
{
class FontLoader : public IAssetLoader
{
public:
    std::shared_ptr<Asset> Create() override;
        bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath, std::string* outError = nullptr) override;
    bool IsAsync() const override { return false; }
};
} // namespace CHEngine

#endif // CH_FONT_LOADER_H
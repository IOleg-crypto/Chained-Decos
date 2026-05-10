#ifndef CH_FONT_LOADER_H
#define CH_FONT_LOADER_H

#include "engine/assets/asset_loader.h"
#include "engine/graphics/assets/font_asset.h"
#include <memory>
#include <string>

namespace CHEngine
{
class FontLoader : public IAssetLoader
{
public:
    std::shared_ptr<Asset> Create() const override;
    bool Load(std::shared_ptr<Asset> asset, const LoadContext& ctx, std::string* outError = nullptr) override;
    bool IsAsync() const override { return false; }
};
} // namespace CHEngine

#endif // CH_FONT_LOADER_H

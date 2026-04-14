#ifndef CH_TEXTURE_LOADER_H
#define CH_TEXTURE_LOADER_H

#include "engine/core/assets/asset_loader.h"
#include "engine/graphics/assets/texture_asset.h"
#include <memory>
#include <string>

namespace CHEngine
{
class TextureLoader : public IAssetLoader
{
public:
    std::shared_ptr<Asset> Create() override;
        bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath, std::string* outError = nullptr) override;
    bool IsAsync() const override { return true; }
};
} // namespace CHEngine

#endif // CH_TEXTURE_LOADER_H

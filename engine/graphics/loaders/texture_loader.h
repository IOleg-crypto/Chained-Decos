#ifndef CH_TEXTURE_LOADER_H
#define CH_TEXTURE_LOADER_H

#include "engine/assets/asset_loader.h"
#include <memory>
#include <string>

namespace CHEngine
{
class TextureLoader : public IAssetLoader
{
public:
    std::shared_ptr<Asset> Create() const override;
    bool Load(std::shared_ptr<Asset> asset, const LoadContext& ctx, std::string* outError = nullptr) override;
    bool IsAsync() const override { return true; }

    static void Finalize(std::shared_ptr<class TextureAsset> asset);
};
} // namespace CHEngine

#endif // CH_TEXTURE_LOADER_H

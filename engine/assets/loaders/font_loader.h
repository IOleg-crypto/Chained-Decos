#ifndef CH_FONT_LOADER_H
#define CH_FONT_LOADER_H

#include "engine/assets/asset_loader.h"
#include "engine/graphics/assets/font_asset.h"
#include <memory>
#include <string>

namespace CHEngine
{
namespace FontLoader
{
    std::shared_ptr<Asset> Create();
    bool Load(std::shared_ptr<Asset> asset, const LoadContext& ctx, std::string* outError = nullptr);

    inline AssetLoaderHooks GetHooks() {
        AssetLoaderHooks hooks;
        hooks.Load = Load;
        hooks.Finalize = nullptr;
        hooks.IsAsync = false;
        return hooks;
    }
}
} // namespace CHEngine

#endif // CH_FONT_LOADER_H

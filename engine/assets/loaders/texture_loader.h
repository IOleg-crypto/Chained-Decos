#ifndef CH_TEXTURE_LOADER_H
#define CH_TEXTURE_LOADER_H

#include "engine/assets/types/texture_asset.h"
#include "engine/assets/loaders/asset_loader.h"
#include "engine/assets/types/texture_asset.h"
#include <memory>
#include <string>

namespace Chained
{
namespace TextureLoader
{
    std::shared_ptr<Asset> Create();
    bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath, std::string* outError = nullptr);
} // namespace TextureLoader
} // namespace Chained

#endif // CH_TEXTURE_LOADER_H
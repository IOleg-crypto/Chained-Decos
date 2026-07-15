#ifndef CH_MATERIAL_LOADER_H
#define CH_MATERIAL_LOADER_H

#include "engine/assets/types/material_asset.h"
#include "engine/assets/loaders/asset_loader.h"
#include <memory>
#include <string>

namespace Chained
{
namespace MaterialLoader
{
    std::shared_ptr<Asset> Create();
    bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath, std::string* outError = nullptr);
} // namespace MaterialLoader
} // namespace Chained

#endif // CH_MATERIAL_LOADER_H

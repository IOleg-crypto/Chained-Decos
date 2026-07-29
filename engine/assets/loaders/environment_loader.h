#ifndef CH_ENVIRONMENT_LOADER_H
#define CH_ENVIRONMENT_LOADER_H

#include "engine/assets/loaders/asset_loader.h"
#include "engine/assets/types/environment_asset.h"
#include <memory>
#include <string>

namespace Chained
{
namespace EnvironmentLoader
{
std::shared_ptr<Asset> Create();
bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath, std::string* outError = nullptr);
} // namespace EnvironmentLoader
} // namespace Chained

#endif // CH_ENVIRONMENT_LOADER_H
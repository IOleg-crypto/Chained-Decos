#ifndef CH_ASSET_LOADER_H
#define CH_ASSET_LOADER_H

#include "engine/assets/asset.h"
#include <memory>
#include <string>
#include <functional>

namespace Chained
{
// Functional struct for asset loaders. Each asset type provides its own functions.
struct AssetLoader
{
    // Creates a new, empty asset of the specific type.
    std::function<std::shared_ptr<Asset>()> Create;

    // Loads the asset data from the given path.
    // For asynchronous loaders, this is called on a background thread.
    std::function<bool(std::shared_ptr<Asset>, const std::string&, std::string*)> Load;

    bool IsAsync = false;
};
} // namespace Chained

#endif // CH_ASSET_LOADER_H
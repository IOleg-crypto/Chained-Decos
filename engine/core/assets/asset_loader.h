#ifndef CH_ASSET_LOADER_H
#define CH_ASSET_LOADER_H

#include "engine/core/assets/asset.h"
#include <memory>
#include <string>

namespace CHEngine
{
// Base interface for asset loaders. Each asset type provides its own loader.
class IAssetLoader
{
public:
    virtual ~IAssetLoader() = default;

    // Creates a new, empty asset of the specific type.
    virtual std::shared_ptr<Asset> Create() = 0;

    // Loads the asset data from the given path.
    // For asynchronous loaders, this is called on a background thread.
    virtual bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath, std::string* outError = nullptr) = 0;

    // Indicates whether Load() should run on a background thread.
    virtual bool IsAsync() const { return false; }
};
} // namespace CHEngine

#endif // CH_ASSET_LOADER_H

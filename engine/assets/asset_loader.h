#ifndef CH_ASSET_LOADER_H
#define CH_ASSET_LOADER_H

#include "engine/assets/asset.h"
#include <memory>
#include <string>

namespace CHEngine
{
struct LoadContext
{
    std::string ResolvedPath;
    AssetHandle Handle;
    // Additional parameters could be added here later (e.g. priority, flags)
};

// Base interface for asset loaders.
class IAssetLoader
{
public:
    virtual ~IAssetLoader() = default;

    // Creates a new empty instance of the asset type this loader handles.
    // This allows AssetManager to hold a typed placeholder during async loads.
    virtual std::shared_ptr<Asset> Create() const = 0;

    // Loads the asset data into the provided asset object.
    // Returns true if loading was successful.
    virtual bool Load(std::shared_ptr<Asset> asset, const LoadContext& ctx, std::string* outError = nullptr) = 0;

    // Indicates whether Load() should run on a background thread.
    virtual bool IsAsync() const
    {
        return false;
    }
};
} // namespace CHEngine

#endif // CH_ASSET_LOADER_H

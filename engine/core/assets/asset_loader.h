#ifndef CH_ASSET_LOADER_H
#define CH_ASSET_LOADER_H

#include "engine/core/assets/asset.h"
#include <memory>
#include <string>

namespace CHEngine
{
/**
 * @brief Base interface for asset loaders.
 * Each asset type (Texture, Model, etc.) implementation will provide its own loader.
 */
class IAssetLoader
{
public:
    virtual ~IAssetLoader() = default;

    /**
     * @brief Creates a new, empty asset of the specific type.
     * @return std::shared_ptr<Asset> The newly created asset.
     */
    virtual std::shared_ptr<Asset> Create() = 0;

    /**
     * @brief Loads the asset data from the given path.
     * For asynchronous loaders, this is called on a background thread.
     * @param asset The asset to load into.
     * @param resolvedPath The absolute/resolved path to the asset file.
     * @return true if loading was successful, false otherwise.
     */
    virtual bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath) = 0;

    /**
     * @brief Indicates if this loader should perform its Load() operation on a background thread.
     * @return true if asynchronous, false if synchronous.
     */
    virtual bool IsAsync() const { return false; }
};
} // namespace CHEngine

#endif // CH_ASSET_LOADER_H

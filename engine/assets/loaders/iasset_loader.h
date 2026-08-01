#ifndef CH_IASSET_LOADER_H
#define CH_IASSET_LOADER_H

#include "engine/assets/asset.h"
#include <memory>
#include <string>

namespace Chained
{
class IAssetLoader
{
public:
    virtual ~IAssetLoader() = default;

    virtual bool IsAsync() const = 0;

    virtual std::shared_ptr<Asset> Create() = 0;

    virtual bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath,
                      std::string* outError = nullptr) = 0;
};
} // namespace Chained

#endif // CH_IASSET_LOADER_H

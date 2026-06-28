#ifndef CH_ENVIRONMENT_LOADER_H
#define CH_ENVIRONMENT_LOADER_H

#include "engine/assets/loaders/asset_loader.h"
#include "engine/assets/types/environment_asset.h"
#include <memory>
#include <string>

namespace Chained
{
class EnvironmentLoader : public IAssetLoader
{
public:
    std::shared_ptr<Asset> Create() override;
        bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath, std::string* outError = nullptr) override;
    bool IsAsync() const override { return true; }
};
} // namespace CHEngine

#endif // CH_ENVIRONMENT_LOADER_H
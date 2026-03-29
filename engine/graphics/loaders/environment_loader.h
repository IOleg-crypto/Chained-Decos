#ifndef CH_ENVIRONMENT_LOADER_H
#define CH_ENVIRONMENT_LOADER_H

#include "engine/core/assets/asset_loader.h"
// #include "engine/graphics/assets/environment.h"
#include <memory>
#include <string>

namespace CHEngine
{
class EnvironmentLoader : public IAssetLoader
{
public:
    std::shared_ptr<Asset> Create() override;
    bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath) override;
    bool IsAsync() const override { return false; }
};
} // namespace CHEngine

#endif // CH_ENVIRONMENT_LOADER_H

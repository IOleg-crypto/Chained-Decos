#ifndef CH_ENVIRONMENT_LOADER_H
#define CH_ENVIRONMENT_LOADER_H

#include "engine/assets/asset_loader.h"
#include "engine/graphics/assets/environment.h"
#include <memory>
#include <string>

namespace CHEngine
{
class EnvironmentLoader : public IAssetLoader
{
public:
    std::shared_ptr<Asset> Create() const override;
    bool Load(std::shared_ptr<Asset> asset, const LoadContext& ctx, std::string* outError = nullptr) override;
    bool IsAsync() const override { return true; }
};
} // namespace CHEngine

#endif // CH_ENVIRONMENT_LOADER_H

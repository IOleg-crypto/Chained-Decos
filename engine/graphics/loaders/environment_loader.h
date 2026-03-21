#ifndef CH_ENVIRONMENT_LOADER_H
#define CH_ENVIRONMENT_LOADER_H

#include "engine/core/assets/asset_loader.h"
#include "engine/graphics/assets/environment.h"
#include "engine/graphics/importers/environment_importer.h"

namespace CHEngine
{
class EnvironmentLoader : public IAssetLoader
{
public:
    std::shared_ptr<Asset> Create() override
    {
        return std::make_shared<EnvironmentAsset>();
    }

    bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath) override
    {
        auto envAsset = std::static_pointer_cast<EnvironmentAsset>(asset);
        auto imported = EnvironmentImporter::ImportEnvironment(resolvedPath);
        if (imported)
        {
            envAsset->SetSettings(imported->GetSettings());
            return true;
        }
        return false;
    }

    bool IsAsync() const override { return false; }
};
} // namespace CHEngine

#endif // CH_ENVIRONMENT_LOADER_H

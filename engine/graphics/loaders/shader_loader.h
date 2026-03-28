#ifndef CH_SHADER_LOADER_H
#define CH_SHADER_LOADER_H

#include "engine/core/assets/asset_loader.h"
#include "engine/graphics/assets/shader_asset.h"
#include "engine/graphics/importers/shader_importer.h"

namespace CHEngine
{
class ShaderLoader : public IAssetLoader
{
public:
    std::shared_ptr<Asset> Create() override
    {
        return std::make_shared<ShaderAsset>();
    }

    bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath) override
    {
        auto shaderAsset = std::static_pointer_cast<ShaderAsset>(asset);
        NativeShader shader = ShaderImporter::LoadShaderFromPath(resolvedPath);
        if (shader.id > 0)
        {
            shaderAsset->SetShader(shader);
            return true;
        }
        return false;
    }

    bool IsAsync() const override { return false; }
};
} // namespace CHEngine

#endif // CH_SHADER_LOADER_H

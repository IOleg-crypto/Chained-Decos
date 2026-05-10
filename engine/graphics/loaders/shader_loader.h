#ifndef CH_SHADER_LOADER_H
#define CH_SHADER_LOADER_H

#include "engine/assets/asset_loader.h"
#include "engine/graphics/assets/shader_asset.h"
#include <memory>
#include <string>
#include <vector>

namespace CHEngine
{

class ShaderLoader : public IAssetLoader
{
public:
    std::shared_ptr<Asset> Create() const override;
    bool Load(std::shared_ptr<Asset> asset, const LoadContext& ctx, std::string* outError = nullptr) override;
    bool IsAsync() const override { return false; }

private:
    std::shared_ptr<Shader> LoadShaderFromPath(const std::string& path, const std::shared_ptr<ShaderAsset>& shaderAsset = nullptr);
    std::shared_ptr<Shader> LoadShaderFromPaths(const std::string& vsPath, const std::string& fsPath);
    std::string ProcessShaderSource(const std::string& path, std::vector<std::string>& includedFiles);
};
} // namespace CHEngine

#endif // CH_SHADER_LOADER_H

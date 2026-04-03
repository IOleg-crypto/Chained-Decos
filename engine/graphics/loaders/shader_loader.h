#ifndef CH_SHADER_LOADER_H
#define CH_SHADER_LOADER_H

#include "engine/core/assets/asset_loader.h"
#include "engine/graphics/assets/shader_asset.h"
#include <memory>
#include <string>
#include <vector>

namespace CHEngine
{

class ShaderLoader : public IAssetLoader
{
public:
    std::shared_ptr<Asset> Create() override;
    bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath) override;
    bool IsAsync() const override { return false; }

private:
    std::shared_ptr<Shader> LoadShaderFromPath(const std::string& path);
    std::shared_ptr<Shader> LoadShaderFromPaths(const std::string& vsPath, const std::string& fsPath);
    std::string ProcessShaderSource(const std::string& path, std::vector<std::string>& includedFiles);
};
} // namespace CHEngine

#endif // CH_SHADER_LOADER_H

#ifndef CH_SHADER_LOADER_H
#define CH_SHADER_LOADER_H

#include "engine/assets/loaders/asset_loader.h"
#include "engine/assets/types/shader_asset.h"
#include <memory>
#include <string>
#include <vector>

namespace Chained
{

namespace ShaderLoader
{
std::shared_ptr<Asset> Create();
bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath, std::string* outError = nullptr);
std::shared_ptr<Shader> LoadShaderFromPath(const std::string& path);
std::shared_ptr<Shader> LoadShaderFromPaths(const std::string& vsPath, const std::string& fsPath);
std::string ProcessShaderSource(const std::string& path, std::vector<std::string>& includedFiles);
} // namespace ShaderLoader
} // namespace Chained

#endif // CH_SHADER_LOADER_H
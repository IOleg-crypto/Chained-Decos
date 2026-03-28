#ifndef CH_SHADER_IMPORTER_H
#define CH_SHADER_IMPORTER_H

#include "engine/graphics/assets/shader_asset.h"
#include <memory>
#include <string>

namespace CHEngine
{
class ShaderImporter
{
public:
    static std::shared_ptr<ShaderAsset> ImportShader(const std::string& path);
    static std::shared_ptr<ShaderAsset> ImportShader(const std::string& vsPath, const std::string& fsPath);
    static NativeShader LoadShaderFromPath(const std::string& path);
    static NativeShader LoadShaderFromPaths(const std::string& vsPath, const std::string& fsPath);

private:
    static std::string ProcessShaderSource(const std::string& path, std::vector<std::string>& includedFiles);
    static NativeShader LoadShaderFromMemory(const char* vsSource, const char* fsSource);
};
} // namespace CHEngine

#endif // CH_SHADER_IMPORTER_H

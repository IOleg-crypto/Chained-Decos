#ifndef CH_SHADER_IMPORTER_H
#define CH_SHADER_IMPORTER_H

#include "engine/graphics/shader_asset.h"
#include <memory>
#include <string>

namespace CHEngine
{
    class ShaderImporter
    {
    public:
        static std::shared_ptr<ShaderAsset> ImportShader(const std::string& path);
        static std::shared_ptr<ShaderAsset> ImportShader(const std::string& vsPath, const std::string& fsPath);
    };
}

#endif // CH_SHADER_IMPORTER_H

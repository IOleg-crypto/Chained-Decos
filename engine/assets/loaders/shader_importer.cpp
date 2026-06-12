#include "engine/assets/asset_manager.h"
#include "engine/assets/loaders/asset_importer.h"
#include <fstream>
#include <sstream>

namespace Chained::AssetImporter
{
std::shared_ptr<ShaderAsset> ImportShader(AssetHandle handle, const AssetMetadata& metadata)
{
    std::filesystem::path fullPath = AssetManager::Get().GetAssetDirectory() / metadata.FilePath;
    std::ifstream in(fullPath, std::ios::in | std::ios::binary);
    if (!in)
    {
        return nullptr;
    }

    std::string result;
    in.seekg(0, std::ios::end);
    size_t size = in.tellg();
    if (size != -1)
    {
        result.resize(size);
        in.seekg(0, std::ios::beg);
        in.read(&result[0], size);
    }
    else
    {
        return nullptr;
    }

    const char* typeToken = "#type";
    size_t typeTokenLength = strlen(typeToken);
    size_t pos = result.find(typeToken, 0);

    std::string vertexSource;
    std::string fragmentSource;

    while (pos != std::string::npos)
    {
        size_t eol = result.find_first_of("\r\n", pos);
        size_t begin = pos + typeTokenLength + 1;
        std::string type = result.substr(begin, eol - begin);

        size_t nextLinePos = result.find_first_not_of("\r\n", eol);
        pos = result.find(typeToken, nextLinePos);

        std::string shaderCode =
            (pos == std::string::npos) ? result.substr(nextLinePos) : result.substr(nextLinePos, pos - nextLinePos);

        if (type == "vertex" || type == "vertex_shader")
        {
            vertexSource = shaderCode;
        }
        else if (type == "fragment" || type == "fragment_shader" || type == "pixel")
        {
            fragmentSource = shaderCode;
        }
    }

    if (vertexSource.empty() || fragmentSource.empty())
    {
        return nullptr;
    }

    auto gpuShader = Shader::Create(vertexSource, fragmentSource);
    if (!gpuShader)
    {
        return nullptr;
    }

    auto asset = std::make_shared<ShaderAsset>(handle);
    asset->SetPath(metadata.FilePath.string());
    asset->SetShader(gpuShader);

    // Uncomment if reflection works currently
    // asset->SetUniforms(gpuShader->GetActiveUniforms());

    asset->SetState(AssetState::Ready);
    return asset;
}
} // namespace Chained::AssetImporter

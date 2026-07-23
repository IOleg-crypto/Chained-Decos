#include "engine/assets/loaders/shader_loader.h"
#include "engine/core/log.h"
#include "engine/core/service_locator.h"
#include "engine/assets/asset_manager.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <yaml-cpp/yaml.h>
#include "engine/assets/types/shader_asset.h"

namespace Chained
{
std::shared_ptr<Asset> ShaderLoader::Create()
{
    return std::make_shared<ShaderAsset>();
}

bool ShaderLoader::Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath, std::string* outError)
{
    auto shaderAsset = std::static_pointer_cast<ShaderAsset>(asset);
    auto shader = LoadShaderFromPath(resolvedPath);
    if (shader)
    {
        shaderAsset->SetShader(shader);
        return true;
    }
    if (outError)
    {
        *outError = "ShaderLoader: failed to load shader from '" + resolvedPath + "'";
    }
    return false;
}

std::shared_ptr<Shader> ShaderLoader::LoadShaderFromPath(const std::string& path)
{
    std::filesystem::path absolutePath(path);

    auto* assetManager = ServiceLocator::TryGet<AssetManager>();
    bool usePack = assetManager && assetManager->IsPacked();

    // Check existence: pack first, then filesystem
    bool exists = false;
    if (usePack)
    {
        uint64_t idx = 0;
        exists = assetManager->ReadAssetData(path).size() > 0;
    }
    else
    {
        exists = std::filesystem::exists(absolutePath);
    }

    if (!exists)
    {
        return nullptr;
    }

    std::string ext = absolutePath.extension().string();
    std::ranges::transform(ext, ext.begin(), ::tolower);

    if (ext == ".chshader")
    {
        try
        {
            YAML::Node config;
            if (usePack)
            {
                auto data = assetManager->ReadAssetData(path);
                std::string content(data.begin(), data.end());
                config = YAML::Load(content);
            }
            else
            {
                config = YAML::LoadFile(absolutePath.string());
            }

            std::string vsRel = config["VertexShader"].as<std::string>();
            std::string fsRel = config["FragmentShader"].as<std::string>();

            std::filesystem::path basePath = absolutePath.parent_path();
            std::string vsPath = (basePath / vsRel).string();
            std::string fsPath = (basePath / fsRel).string();

            return LoadShaderFromPaths(vsPath, fsPath);
        } catch (...)
        {
            return nullptr;
        }
    }
    else if (ext == ".vs" || ext == ".vert" || ext == ".glsl")
    {
        std::filesystem::path fsPath = absolutePath;
        fsPath.replace_extension(".fs");
        if (!std::filesystem::exists(fsPath))
        {
            fsPath.replace_extension(".frag");
        }
        if (std::filesystem::exists(fsPath))
        {
            return LoadShaderFromPaths(absolutePath.string(), fsPath.string());
        }
    }
    return nullptr;
}

std::shared_ptr<Shader> ShaderLoader::LoadShaderFromPaths(const std::string& vsPath, const std::string& fsPath)
{
    std::vector<std::string> vsIncl, fsIncl;
    std::string vsSource = ProcessShaderSource(vsPath, vsIncl);
    std::string fsSource = ProcessShaderSource(fsPath, fsIncl);

    return Shader::Create(vsSource.c_str(), fsSource.c_str());
}

std::string ShaderLoader::ProcessShaderSource(const std::string& path, std::vector<std::string>& includedFiles)
{
    std::filesystem::path fullPath = std::filesystem::absolute(path);

    for (const auto& included : includedFiles)
    {
        if (included == fullPath.string())
        {
            CH_CORE_WARN("ShaderPreprocessor: Circular include detected: {}", path);
            return "";
        }
    }
    includedFiles.push_back(fullPath.string());

    auto* assetManager = ServiceLocator::TryGet<AssetManager>();
    bool usePack = assetManager && assetManager->IsPacked();

    // Check existence
    bool exists = false;
    if (usePack)
    {
        auto data = assetManager->ReadAssetData(path);
        exists = !data.empty();
    }
    else
    {
        exists = std::filesystem::exists(fullPath);
    }

    if (!exists)
    {
        CH_CORE_ERROR("ShaderPreprocessor: File not found: {}", path);
        return "";
    }

    // Read file content
    std::string fileContent;
    if (usePack)
    {
        auto data = assetManager->ReadAssetData(path);
        fileContent.assign(data.begin(), data.end());
    }
    else
    {
        std::ifstream file(fullPath);
        if (!file.is_open())
        {
            CH_CORE_ERROR("ShaderPreprocessor: Cannot open file: {}", path);
            return "";
        }
        std::stringstream ss;
        ss << file.rdbuf();
        fileContent = ss.str();
    }

    // Process includes
    std::stringstream ss;
    std::regex includeRegex(R"(^\s*#include\s+["<](.*)[">])");
    std::smatch match;
    std::istringstream stream(fileContent);
    std::string line;

    while (std::getline(stream, line))
    {
        if (std::regex_search(line, match, includeRegex))
        {
            std::string includeFile = match[1].str();
            std::filesystem::path includePath = fullPath.parent_path() / includeFile;

            std::string includedSource = ProcessShaderSource(includePath.string(), includedFiles);
            if (includedSource.empty() && std::filesystem::exists(includePath))
            {
                CH_CORE_WARN("ShaderPreprocessor: Failed to process include: {}", includeFile);
            }
            ss << includedSource << "\n";
        }
        else
        {
            ss << line << "\n";
        }
    }

    return ss.str();
}
} // namespace Chained
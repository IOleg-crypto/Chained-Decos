#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "engine/assets/loaders/asset_importer.h"
#include "engine/core/log.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <regex>
#include <yaml-cpp/yaml.h>

namespace Chained::AssetImporter
{
namespace
{
    std::string ProcessShaderSource(const std::string& path, std::vector<std::string>& includedFiles)
    {
        std::filesystem::path fullPath = std::filesystem::absolute(path);

        for (const auto& included : includedFiles)
        {
            if (included == fullPath.string())
            {
                return ""; // Prevent circular includes
            }
        }
        includedFiles.push_back(fullPath.string());

        if (!std::filesystem::exists(fullPath))
        {
            CH_CORE_ERROR("ShaderPreprocessor: File not found: {}", path);
            return "";
        }

        std::ifstream file(fullPath);
        if (!file.is_open())
        {
            return "";
        }

        std::stringstream ss;
        std::string line;
        std::regex includeRegex(R"(^\s*#include\s+["<](.*)[">])");
        std::smatch match;

        while (std::getline(file, line))
        {
            if (std::regex_search(line, match, includeRegex))
            {
                std::string includeFile = match[1].str();
                std::filesystem::path includePath = fullPath.parent_path() / includeFile;
                ss << ProcessShaderSource(includePath.string(), includedFiles) << "\n";
            }
            else
            {
                ss << line << "\n";
            }
        }

        return ss.str();
    }

    std::shared_ptr<Shader> LoadShaderFromPaths(const std::string& vsPath, const std::string& fsPath)
    {
        std::vector<std::string> vsIncl, fsIncl;
        std::string vsSource = ProcessShaderSource(vsPath, vsIncl);
        std::string fsSource = ProcessShaderSource(fsPath, fsIncl);

        if (vsSource.empty() || fsSource.empty())
        {
            return nullptr;
        }

        return Shader::Create(vsSource, fsSource);
    }
}

std::shared_ptr<ShaderAsset> ImportShader(AssetHandle handle, const AssetMetadata& metadata)
{
    // The FilePath is already resolved by AssetManager to an absolute or valid path
    std::filesystem::path absolutePath = metadata.FilePath;
    if (!absolutePath.is_absolute())
    {
        // Fallback just in case
        absolutePath = ServiceLocator::Get<AssetManager>()->GetAssetDirectory() / absolutePath;
    }

    if (!std::filesystem::exists(absolutePath))
    {
        CH_CORE_ERROR("ImportShader failed: {} does not exist", absolutePath.string());
        return nullptr;
    }

    std::string ext = absolutePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    std::shared_ptr<Shader> gpuShader = nullptr;

    if (ext == ".chshader")
    {
        try
        {
            YAML::Node config = YAML::LoadFile(absolutePath.string());
            std::string vsRel = config["VertexShader"].as<std::string>();
            std::string fsRel = config["FragmentShader"].as<std::string>();

            std::filesystem::path basePath = absolutePath.parent_path();
            std::string vsPath = (basePath / vsRel).string();
            std::string fsPath = (basePath / fsRel).string();

            gpuShader = LoadShaderFromPaths(vsPath, fsPath);
        }
        catch (const std::exception& e)
        {
            CH_CORE_ERROR("Failed to parse .chshader file {}: {}", absolutePath.string(), e.what());
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
            gpuShader = LoadShaderFromPaths(absolutePath.string(), fsPath.string());
        }
        else
        {
            CH_CORE_ERROR("Could not find matching fragment shader for {}", absolutePath.string());
        }
    }
    else
    {
        // Legacy monolithic format fallback (with #type vertex and #type fragment)
        std::ifstream in(absolutePath, std::ios::in | std::ios::binary);
        if (in)
        {
            std::string result;
            in.seekg(0, std::ios::end);
            size_t size = in.tellg();
            if (size != -1)
            {
                result.resize(size);
                in.seekg(0, std::ios::beg);
                in.read(&result[0], size);
                
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

                    std::string shaderCode = (pos == std::string::npos) ? result.substr(nextLinePos) : result.substr(nextLinePos, pos - nextLinePos);

                    if (type == "vertex" || type == "vertex_shader") vertexSource = shaderCode;
                    else if (type == "fragment" || type == "fragment_shader" || type == "pixel") fragmentSource = shaderCode;
                }

                if (!vertexSource.empty() && !fragmentSource.empty())
                {
                    gpuShader = Shader::Create(vertexSource, fragmentSource);
                }
            }
        }
    }

    if (!gpuShader)
    {
        return nullptr;
    }

    auto asset = std::make_shared<ShaderAsset>(handle);
    asset->SetPath(metadata.FilePath.string());
    asset->SetShader(gpuShader);
    asset->SetState(AssetState::Ready);
    
    return asset;
}
} // namespace Chained::AssetImporter

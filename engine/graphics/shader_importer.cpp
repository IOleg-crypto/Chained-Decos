#include "shader_importer.h"
#include "engine/core/log.h"
#include "yaml-cpp/yaml.h"
#include <filesystem>

#include <fstream>
#include <sstream>
#include <regex>

namespace CHEngine
{
    static std::string ProcessShaderSource(const std::string& path, std::vector<std::string>& includedFiles)
    {
        std::filesystem::path fullPath = std::filesystem::absolute(path);
        
        // Prevent circular includes
        for (const auto& included : includedFiles) {
            if (included == fullPath.string()) return "";
        }
        includedFiles.push_back(fullPath.string());

        if (!std::filesystem::exists(fullPath)) {
            CH_CORE_ERROR("ShaderPreprocessor: File not found: {}", path);
            return "";
        }

        std::ifstream file(fullPath);
        if (!file.is_open()) {
            CH_CORE_ERROR("ShaderPreprocessor: Could not open file: {}", path);
            return "";
        }

        std::stringstream ss;
        std::string line;
        std::regex includeRegex(R"(^\s*#include\s+["<](.*)[">])");
        std::smatch match;

        while (std::getline(file, line)) {
            if (std::regex_search(line, match, includeRegex)) {
                std::string includeFile = match[1].str();
                std::filesystem::path includePath = fullPath.parent_path() / includeFile;
                ss << ProcessShaderSource(includePath.string(), includedFiles) << "\n";
            } else {
                ss << line << "\n";
            }
        }

        return ss.str();
    }

    std::shared_ptr<ShaderAsset> ShaderImporter::ImportShader(const std::string& path)
    {
        std::filesystem::path absolutePath(path);
        if (!std::filesystem::exists(absolutePath))
        {
            CH_CORE_ERROR("ShaderImporter: File not found: {}", path);
            return nullptr;
        }

        std::string ext = absolutePath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        // If it's a .chshader file, parse it as YAML
        if (ext == ".chshader")
        {
            try
            {
                YAML::Node config = YAML::LoadFile(absolutePath.string());
                std::string vertexShaderRelative = config["VertexShader"].as<std::string>();
                std::string fragmentShaderRelative = config["FragmentShader"].as<std::string>();

                std::filesystem::path basePath = absolutePath.parent_path();
                std::string vsPath = (basePath / vertexShaderRelative).string();
                std::string fsPath = (basePath / fragmentShaderRelative).string();

                std::vector<std::string> vsIncludes, fsIncludes;
                std::string vsSource = ProcessShaderSource(vsPath, vsIncludes);
                std::string fsSource = ProcessShaderSource(fsPath, fsIncludes);

                Shader shader = ::LoadShaderFromMemory(vsSource.c_str(), fsSource.c_str());
                if (shader.id == 0)
                {
                    CH_CORE_ERROR("ShaderImporter: Failed to load shader from memory: VS: {}, FS: {}", vsPath, fsPath);
                    return nullptr;
                }

                auto asset = std::make_shared<ShaderAsset>(shader);
                asset->SetPath(path);

                // Map uniforms if present
                if (config["Uniforms"])
                {
                    for (auto uniform : config["Uniforms"])
                    {
                        std::string name = uniform.as<std::string>();
                        int location = asset->GetLocation(name);

                        if (name == "mvp") asset->GetShader().locs[SHADER_LOC_MATRIX_MVP] = location;
                        else if (name == "matModel") asset->GetShader().locs[SHADER_LOC_MATRIX_MODEL] = location;
                        else if (name == "matNormal") asset->GetShader().locs[SHADER_LOC_MATRIX_NORMAL] = location;
                        else if (name == "matView") asset->GetShader().locs[SHADER_LOC_MATRIX_VIEW] = location;
                        else if (name == "matProjection") asset->GetShader().locs[SHADER_LOC_MATRIX_PROJECTION] = location;
                        else if (name == "viewPos") asset->GetShader().locs[SHADER_LOC_VECTOR_VIEW] = location;
                        else if (name == "texture0") asset->GetShader().locs[SHADER_LOC_MAP_DIFFUSE] = location;
                        else if (name == "colDiffuse") asset->GetShader().locs[SHADER_LOC_COLOR_DIFFUSE] = location;
                        else if (name == "panorama") asset->GetShader().locs[SHADER_LOC_MAP_ALBEDO] = location;
                        else if (name == "environmentMap") asset->GetShader().locs[SHADER_LOC_MAP_CUBEMAP] = location;
                        else if (name == "boneMatrices") asset->GetShader().locs[SHADER_LOC_BONE_MATRICES] = location;
                    }
                }

                asset->SetState(AssetState::Ready);
                return asset;
            }
            catch (const std::exception& e)
            {
                CH_CORE_ERROR("ShaderImporter: Failed to parse .chshader {}: {}", path, e.what());
                return nullptr;
            }
        }
        else if (ext == ".vs" || ext == ".vert" || ext == ".glsl")
        {
            // Try to find a matching .fs or .frag
            std::filesystem::path fsPath = absolutePath;
            fsPath.replace_extension(".fs");
            if (!std::filesystem::exists(fsPath)) fsPath.replace_extension(".frag");
            
            if (std::filesystem::exists(fsPath))
            {
                return ImportShader(absolutePath.string(), fsPath.string());
            }
        }
        else if (ext == ".fs" || ext == ".frag")
        {
            // Try to find a matching .vs or .vert
            std::filesystem::path vsPath = absolutePath;
            vsPath.replace_extension(".vs");
            if (!std::filesystem::exists(vsPath)) vsPath.replace_extension(".vert");
            
            if (std::filesystem::exists(vsPath))
            {
                return ImportShader(vsPath.string(), absolutePath.string());
            }
        }
        
        return nullptr;
    }

    std::shared_ptr<ShaderAsset> ShaderImporter::ImportShader(const std::string& vsPath, const std::string& fsPath)
    {
        std::vector<std::string> vsIncludes, fsIncludes;
        std::string vsSource = ProcessShaderSource(vsPath, vsIncludes);
        std::string fsSource = ProcessShaderSource(fsPath, fsIncludes);

        Shader shader = ::LoadShaderFromMemory(vsSource.c_str(), fsSource.c_str());
        if (shader.id > 0)
        {
            auto asset = std::make_shared<ShaderAsset>(shader);
            asset->SetPath(vsPath + "|" + fsPath);
            asset->SetState(AssetState::Ready);
            return asset;
        }

        CH_CORE_ERROR("ShaderImporter: Failed to load shader from memory: VS: {}, FS: {}", vsPath, fsPath);
        return nullptr;
    }
}

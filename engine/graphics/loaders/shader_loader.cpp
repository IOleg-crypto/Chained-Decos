#include "engine/graphics/loaders/shader_loader.h"
#include "engine/core/log.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <yaml-cpp/yaml.h>
#include <engine/graphics/assets/shader_asset.h>
#include "engine/graphics/pipeline/renderer_types.h"

namespace CHEngine
{
    std::shared_ptr<Asset> ShaderLoader::Create() const
    {
        return std::make_shared<ShaderAsset>();
    }

    bool ShaderLoader::Load(std::shared_ptr<Asset> asset, const LoadContext& ctx, std::string* outError)
    {
        auto shaderAsset = std::dynamic_pointer_cast<ShaderAsset>(asset);
        if (!shaderAsset)
        {
            if (outError) *outError = "ShaderLoader: Invalid asset type";
            return false;
        }

        auto shader = LoadShaderFromPath(ctx.ResolvedPath, shaderAsset);
        if (shader)
        {
            shaderAsset->SetShader(shader);
            return true;
        }
        if (outError)
        {
            *outError = "ShaderLoader: failed to load shader from '" + ctx.ResolvedPath + "'";
        }
        return false;
    }

    std::shared_ptr<Shader> ShaderLoader::LoadShaderFromPath(const std::string& path, const std::shared_ptr<ShaderAsset>& shaderAsset)
    {
        std::filesystem::path absolutePath(path);
        if (!std::filesystem::exists(absolutePath))
        {
            return nullptr;
        }

        std::string ext = absolutePath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

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

                auto shader = LoadShaderFromPaths(vsPath, fsPath);
                if (shader && shaderAsset)
                {
                    if (config["Uniforms"])
                    {
                        std::vector<ShaderUniform> uniforms;
                        for (auto u : config["Uniforms"])
                        {
                            if (u.IsScalar())
                            {
                                // Backwards compatibility: just string
                                ShaderUniform unif;
                                unif.Name = u.as<std::string>();
                                unif.Type = 0; // Float
                                uniforms.push_back(unif);
                            }
                            else if (u.IsMap())
                            {
                                ShaderUniform unif;
                                unif.Name = u["Name"].as<std::string>();
                                
                                std::string typeStr = "Float";
                                if (u["Type"]) typeStr = u["Type"].as<std::string>();
                                
                                if (typeStr == "Float") unif.Type = 0;
                                else if (typeStr == "Vec2") unif.Type = 1;
                                else if (typeStr == "Vec3") unif.Type = 2;
                                else if (typeStr == "Vec4") unif.Type = 3;
                                else if (typeStr == "Color") unif.Type = 4;
                                else unif.Type = 0; // fallback

                                if (u["Default"] && u["Default"].IsSequence())
                                {
                                    int i = 0;
                                    for (auto elem : u["Default"])
                                    {
                                        if (i < 4) unif.Value[i] = elem.as<float>();
                                        i++;
                                    }
                                }
                                else if (u["Default"] && u["Default"].IsScalar())
                                {
                                    unif.Value[0] = u["Default"].as<float>();
                                }

                                uniforms.push_back(unif);
                            }
                        }
                        shaderAsset->SetUniforms(uniforms);
                    }
                }
                return shader;
            } catch (const std::exception& e)
            {
                CH_CORE_ERROR("ShaderLoader: Error parsing {}: {}", absolutePath.string(), e.what());
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
                return "";
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
} // namespace CHEngine

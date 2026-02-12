#include "shader_importer.h"
#include "engine/core/log.h"
#include "yaml-cpp/yaml.h"
#include <filesystem>

namespace CHEngine
{
    std::shared_ptr<ShaderAsset> ShaderImporter::ImportShader(const std::string& path)
    {
        std::filesystem::path absolutePath(path);
        if (!std::filesystem::exists(absolutePath))
        {
            CH_CORE_ERROR("ShaderImporter: File not found: {}", path);
            return nullptr;
        }

        // If it's a .chshader file, parse it as YAML
        if (absolutePath.extension() == ".chshader")
        {
            try
            {
                YAML::Node config = YAML::LoadFile(absolutePath.string());
                std::string vertexShaderRelative = config["VertexShader"].as<std::string>();
                std::string fragmentShaderRelative = config["FragmentShader"].as<std::string>();

                std::filesystem::path basePath = absolutePath.parent_path();
                std::string vsPath = (basePath / vertexShaderRelative).string();
                std::string fsPath = (basePath / fragmentShaderRelative).string();

                Shader shader = ::LoadShader(vsPath.c_str(), fsPath.c_str());
                if (shader.id == 0)
                {
                    CH_CORE_ERROR("ShaderImporter: Failed to load shader: VS: {}, FS: {}", vsPath, fsPath);
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

                    asset->GetShader().locs[SHADER_LOC_VERTEX_BONEIDS] = GetShaderLocationAttrib(asset->GetShader(), "vertexBoneIds");
                    asset->GetShader().locs[SHADER_LOC_VERTEX_BONEWEIGHTS] = GetShaderLocationAttrib(asset->GetShader(), "vertexBoneWeights");
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
        
        // Fallback or other formats could be added here
        return nullptr;
    }

    std::shared_ptr<ShaderAsset> ShaderImporter::ImportShader(const std::string& vsPath, const std::string& fsPath)
    {
        Shader shader = ::LoadShader(vsPath.c_str(), fsPath.c_str());
        if (shader.id > 0)
        {
            auto asset = std::make_shared<ShaderAsset>(shader);
            asset->SetPath(vsPath + "|" + fsPath);
            asset->SetState(AssetState::Ready);
            return asset;
        }

        CH_CORE_ERROR("ShaderImporter: Failed to load shader: VS: {}, FS: {}", vsPath, fsPath);
        return nullptr;
    }
}

#include "engine/graphics/importers/shader_importer.h"
#include "engine/core/log.h"
#include "yaml-cpp/yaml.h"
#include <filesystem>

#include <fstream>
#include <regex>
#include <sstream>

namespace CHEngine
{
std::string ShaderImporter::ProcessShaderSource(const std::string& path, std::vector<std::string>& includedFiles)
{
    std::filesystem::path fullPath = std::filesystem::absolute(path);

    // Prevent circular includes
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
        CH_CORE_ERROR("ShaderPreprocessor: Could not open file: {}", path);
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

std::shared_ptr<ShaderAsset> ShaderImporter::ImportShader(const std::string& path)
{
    Shader shader = LoadShaderFromPath(path);
    if (shader.id == 0)
    {
        return nullptr;
    }

    auto asset = std::make_shared<ShaderAsset>(shader);
    asset->SetPath(path);
    return asset;
}

Shader ShaderImporter::LoadShaderFromPath(const std::string& path)
{
    std::filesystem::path absolutePath(path);
    if (!std::filesystem::exists(absolutePath))
    {
        CH_CORE_ERROR("ShaderImporter: File not found: {}", path);
        return { 0 };
    }

    std::string ext = absolutePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    Shader shader = { 0 };

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

            shader = ::LoadShaderFromMemory(vsSource.c_str(), fsSource.c_str());
            if (shader.id == 0)
            {
                CH_CORE_ERROR("ShaderImporter: Failed to load shader from memory: VS: {}, FS: {}", vsPath, fsPath);
                return { 0 };
            }

            // --- Automatic Mapping of Standard Uniforms ---
            // We always try to find these common names to ensure raylib's internal
            // model drawing logic (DrawMesh, etc.) can set them automatically.
            auto& locs = shader.locs;

            auto GetLoc = [&](const std::string& name) -> int {
                return ::GetShaderLocation(shader, name.c_str());
            };

            auto MapStandard = [&](int locIndex, const std::string& name) {
                int loc = GetLoc(name);
                if (loc >= 0)
                {
                    locs[locIndex] = loc;
                }
            };

            MapStandard(SHADER_LOC_MATRIX_MVP, "mvp");
            MapStandard(SHADER_LOC_MATRIX_MODEL, "matModel");
            MapStandard(SHADER_LOC_MATRIX_NORMAL, "matNormal");
            MapStandard(SHADER_LOC_MATRIX_VIEW, "matView");
            MapStandard(SHADER_LOC_MATRIX_PROJECTION, "matProjection");
            MapStandard(SHADER_LOC_VECTOR_VIEW, "viewPos");
            MapStandard(SHADER_LOC_MAP_ALBEDO, "texture0");
            MapStandard(SHADER_LOC_MAP_METALNESS, "texture1");
            MapStandard(SHADER_LOC_MAP_NORMAL, "texture2");
            MapStandard(SHADER_LOC_MAP_ROUGHNESS, "texture3");
            MapStandard(SHADER_LOC_MAP_OCCLUSION, "texture4");
            MapStandard(SHADER_LOC_MAP_EMISSION, "texture5");
            MapStandard(SHADER_LOC_COLOR_DIFFUSE, "colDiffuse");
            MapStandard(SHADER_LOC_BONE_MATRICES, "boneMatrices");

            // --- Attribute Mappings ---
            // These must match the 'in' variables in the vertex shader (e.g., lighting.vs)
            MapStandard(SHADER_LOC_VERTEX_BONEIDS, "vertexBoneIds");
            MapStandard(SHADER_LOC_VERTEX_BONEWEIGHTS, "vertexBoneWeights");

            // Legacy/Specific mappings
            int panoLoc = GetLoc("panorama");
            if (panoLoc >= 0)
            {
                locs[SHADER_LOC_MAP_ALBEDO] = panoLoc;
            }

            int envLoc = GetLoc("environmentMap");
            if (envLoc >= 0)
            {
                locs[SHADER_LOC_MAP_CUBEMAP] = envLoc;
            }

            // --- Manual Override from metadata ---
            // (In case the user used non-standard names but wants them mapped to standard slots)
            if (config["Uniforms"])
            {
                for (auto uniform : config["Uniforms"])
                {
                    std::string name = uniform.as<std::string>();
                    int location = GetLoc(name);
                    if (location < 0)
                    {
                        continue;
                    }

                    if (name == "mvp")
                    {
                        locs[SHADER_LOC_MATRIX_MVP] = location;
                    }
                    else if (name == "matModel")
                    {
                        locs[SHADER_LOC_MATRIX_MODEL] = location;
                    }
                    else if (name == "matNormal")
                    {
                        locs[SHADER_LOC_MATRIX_NORMAL] = location;
                    }
                    else if (name == "viewPos")
                    {
                        locs[SHADER_LOC_VECTOR_VIEW] = location;
                    }
                }
            }

            return shader;
        } catch (const std::exception& e)
        {
            CH_CORE_ERROR("ShaderImporter: Failed to parse .chshader {}: {}", path, e.what());
            return { 0 };
        }
    }
    else if (ext == ".vs" || ext == ".vert" || ext == ".glsl")
    {
        // Try to find a matching .fs or .frag
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
    else if (ext == ".fs" || ext == ".frag")
    {
        // Try to find a matching .vs or .vert
        std::filesystem::path vsPath = absolutePath;
        vsPath.replace_extension(".vs");
        if (!std::filesystem::exists(vsPath))
        {
            vsPath.replace_extension(".vert");
        }

        if (std::filesystem::exists(vsPath))
        {
            return LoadShaderFromPaths(vsPath.string(), absolutePath.string());
        }
    }

    return { 0 };
}

std::shared_ptr<ShaderAsset> ShaderImporter::ImportShader(const std::string& vsPath, const std::string& fsPath)
{
    Shader shader = LoadShaderFromPaths(vsPath, fsPath);
    if (shader.id > 0)
    {
        auto asset = std::make_shared<ShaderAsset>(shader);
        asset->SetPath(vsPath + "|" + fsPath);
        return asset;
    }

    return nullptr;
}

Shader ShaderImporter::LoadShaderFromPaths(const std::string& vsPath, const std::string& fsPath)
{
    std::vector<std::string> vsIncludes, fsIncludes;
    std::string vsSource = ProcessShaderSource(vsPath, vsIncludes);
    std::string fsSource = ProcessShaderSource(fsPath, fsIncludes);

    Shader shader = ::LoadShaderFromMemory(vsSource.c_str(), fsSource.c_str());
    if (shader.id == 0)
    {
        CH_CORE_ERROR("ShaderImporter: Failed to load shader from memory: VS: {}, FS: {}", vsPath, fsPath);
    }
    
    return shader;
}
} // namespace CHEngine

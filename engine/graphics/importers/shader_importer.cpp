#include "engine/graphics/importers/shader_importer.h"
#include "engine/core/log.h" // Mouse buttons comment fix (original instruction was ambiguous, assuming this was the intent)
#include "yaml-cpp/yaml.h"
#include <glad/gl.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <vector>
#include <algorithm>

namespace CHEngine
{
std::string ShaderImporter::ProcessShaderSource(const std::string& path, std::vector<std::string>& includedFiles)
{
    std::filesystem::path fullPath = std::filesystem::absolute(path);

    for (const auto& included : includedFiles)
    {
        if (included == fullPath.string()) return "";
    }
    includedFiles.push_back(fullPath.string());

    if (!std::filesystem::exists(fullPath))
    {
        CH_CORE_ERROR("ShaderPreprocessor: File not found: {}", path);
        return "";
    }

    std::ifstream file(fullPath);
    if (!file.is_open()) return "";

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
        else ss << line << "\n";
    }

    return ss.str();
}

std::shared_ptr<ShaderAsset> ShaderImporter::ImportShader(const std::string& path)
{
    NativeShader shader = LoadShaderFromPath(path);
    if (shader.id == 0) return nullptr;

    auto asset = std::make_shared<ShaderAsset>(shader);
    asset->SetPath(path);
    return asset;
}

NativeShader ShaderImporter::LoadShaderFromPath(const std::string& path)
{
    std::filesystem::path absolutePath(path);
    if (!std::filesystem::exists(absolutePath)) return { 0 };

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

            return LoadShaderFromPaths(vsPath, fsPath);
        } catch (...) { return {0}; }
    }
    else if (ext == ".vs" || ext == ".vert" || ext == ".glsl")
    {
        std::filesystem::path fsPath = absolutePath;
        fsPath.replace_extension(".fs");
        if (!std::filesystem::exists(fsPath)) fsPath.replace_extension(".frag");
        if (std::filesystem::exists(fsPath)) return LoadShaderFromPaths(absolutePath.string(), fsPath.string());
    }
    return { 0 };
}

std::shared_ptr<ShaderAsset> ShaderImporter::ImportShader(const std::string& vsPath, const std::string& fsPath)
{
    NativeShader shader = LoadShaderFromPaths(vsPath, fsPath);
    if (shader.id > 0)
    {
        auto asset = std::make_shared<ShaderAsset>(shader);
        asset->SetPath(vsPath + "|" + fsPath);
        return asset;
    }
    return nullptr;
}

NativeShader ShaderImporter::LoadShaderFromPaths(const std::string& vsPath, const std::string& fsPath)
{
    std::vector<std::string> vsIncl, fsIncl;
    std::string vsSource = ProcessShaderSource(vsPath, vsIncl);
    std::string fsSource = ProcessShaderSource(fsPath, fsIncl);

    return LoadShaderFromMemory(vsSource.c_str(), fsSource.c_str());
}

NativeShader ShaderImporter::LoadShaderFromMemory(const char* vsSource, const char* fsSource)
{
    auto compileShader = [](GLenum type, const char* source) -> uint32_t {
        uint32_t shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        int success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            CH_CORE_ERROR("Shader Compilation Error: {}", infoLog);
            return 0;
        }
        return shader;
    };

    uint32_t vs = compileShader(GL_VERTEX_SHADER, vsSource);
    uint32_t fs = compileShader(GL_FRAGMENT_SHADER, fsSource);

    if (vs == 0 || fs == 0) return { 0 };

    uint32_t program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        CH_CORE_ERROR("Shader Linking Error: {}", infoLog);
        return { 0 };
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return { program };
}
} // namespace CHEngine

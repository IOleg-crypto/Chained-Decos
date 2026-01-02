#ifndef CD_COMPONENTS_RENDERING_SHADER_SHADER_MANAGER_H
#define CD_COMPONENTS_RENDERING_SHADER_SHADER_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <raylib.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>

class ShaderManager
{
private:
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_shaders;
    std::string m_VertexShaderPath;
    std::string m_FragmentShaderPath;

public:
    ShaderManager();
    ~ShaderManager();
    bool LoadVertexShader(const std::string& name, const std::string& vertexShaderPath);
    bool LoadFragmentShader(const std::string& name, const std::string& fragmentShaderPath);
    bool LoadShaderPair(const std::string& name, const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
    bool UnloadVertexShader(const std::string& name);
    bool UnloadFragmentShader(const std::string& name);
    bool UnloadShader(const std::string& name);
    bool UnloadAllShaders();
    bool IsVertexShaderLoaded(const std::string& name) const;
    bool IsFragmentShaderLoaded(const std::string& name) const;
    bool IsShaderLoaded(const std::string& name) const;
    Shader* GetVertexShader(const std::string& name);
    Shader* GetFragmentShader(const std::string& name);
    Shader* GetShader(const std::string& name);
};

#endif /* shader_manager.h */





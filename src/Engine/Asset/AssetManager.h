#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <raylib.h>

class AssetManager
{
public:
    AssetManager() = default;
    ~AssetManager() = default;

    // Asset loading with caching
    bool LoadTexture(const std::string& name, const std::string& filePath);
    bool LoadFont(const std::string& name, const std::string& filePath, int fontSize = 32);
    bool LoadModel(const std::string& name, const std::string& filePath);
    bool LoadShader(const std::string& name, const std::string& vsFilePath, const std::string& fsFilePath);

    // Asset retrieval
    Texture2D* GetTexture(const std::string& name);
    Font* GetFont(const std::string& name);
    Model* GetModel(const std::string& name);
    Shader* GetShader(const std::string& name);

    // Asset existence check
    bool HasTexture(const std::string& name) const;
    bool HasFont(const std::string& name) const;
    bool HasModel(const std::string& name) const;
    bool HasShader(const std::string& name) const;

    // Asset unloading
    void UnloadTexture(const std::string& name);
    void UnloadFont(const std::string& name);
    void UnloadModel(const std::string& name);
    void UnloadShader(const std::string& name);
    void UnloadAll();

    // Asset information
    size_t GetTextureCount() const { return m_textures.size(); }
    size_t GetFontCount() const { return m_fonts.size(); }
    size_t GetModelCount() const { return m_models.size(); }
    size_t GetShaderCount() const { return m_shaders.size(); }

    // Memory usage estimation (rough)
    size_t GetEstimatedMemoryUsage() const;

private:
    std::unordered_map<std::string, std::shared_ptr<Texture2D>> m_textures;
    std::unordered_map<std::string, std::shared_ptr<Font>> m_fonts;
    std::unordered_map<std::string, std::shared_ptr<Model>> m_models;
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_shaders;

    // Helper function to check if file exists
    bool FileExists(const std::string& filePath) const;
};

#endif // ASSETMANAGER_H
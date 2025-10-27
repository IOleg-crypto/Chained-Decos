#include "AssetManager.h"
#include <iostream>
#include <fstream>

/**
 * Loads a texture asset with caching and error handling
 * @param name Unique identifier for the texture
 * @param filePath Path to the texture file
 * @return true if loaded successfully (or already cached), false on error
 */
bool AssetManager::LoadTexture(const std::string& name, const std::string& filePath)
{
    // Check if already loaded
    if (m_textures.find(name) != m_textures.end())
    {
        TraceLog(LOG_INFO, "AssetManager::LoadTexture() - Texture '%s' already cached", name.c_str());
        return true;
    }

    // Validate file exists
    if (!FileExists(filePath))
    {
        TraceLog(LOG_ERROR, "AssetManager::LoadTexture() - Texture file not found: %s", filePath.c_str());
        return false;
    }

    // Load texture with error checking
    Texture2D texture = LoadTexture(filePath.c_str());
    if (texture.id == 0)
    {
        TraceLog(LOG_ERROR, "AssetManager::LoadTexture() - Failed to load texture: %s", filePath.c_str());
        return false;
    }

    // Cache the texture
    m_textures[name] = std::make_shared<Texture2D>(texture);
    TraceLog(LOG_INFO, "AssetManager::LoadTexture() - Loaded texture '%s' (%dx%d) from %s",
             name.c_str(), texture.width, texture.height, filePath.c_str());
    return true;
}

bool AssetManager::LoadFont(const std::string& name, const std::string& filePath, int fontSize)
{
    if (m_fonts.find(name) != m_fonts.end())
    {
        TraceLog(LOG_INFO, "AssetManager::LoadFont() - Font '%s' already cached", name.c_str());
        return true;
    }

    if (!FileExists(filePath))
    {
        TraceLog(LOG_ERROR, "AssetManager::LoadFont() - Font file not found: %s", filePath.c_str());
        return false;
    }

    Font font = LoadFontEx(filePath.c_str(), fontSize, nullptr, 0);
    if (font.texture.id == 0)
    {
        TraceLog(LOG_ERROR, "AssetManager::LoadFont() - Failed to load font: %s", filePath.c_str());
        return false;
    }

    m_fonts[name] = std::make_shared<Font>(font);
    TraceLog(LOG_INFO, "AssetManager::LoadFont() - Loaded font '%s' from %s (size: %d)",
             name.c_str(), filePath.c_str(), fontSize);
    return true;
}

bool AssetManager::LoadModel(const std::string& name, const std::string& filePath)
{
    if (m_models.find(name) != m_models.end())
    {
        TraceLog(LOG_INFO, "AssetManager::LoadModel() - Model '%s' already cached", name.c_str());
        return true;
    }

    if (!FileExists(filePath))
    {
        TraceLog(LOG_ERROR, "AssetManager::LoadModel() - Model file not found: %s", filePath.c_str());
        return false;
    }

    Model model = LoadModel(filePath.c_str());
    if (model.meshCount == 0)
    {
        TraceLog(LOG_ERROR, "AssetManager::LoadModel() - Failed to load model: %s", filePath.c_str());
        return false;
    }

    m_models[name] = std::make_shared<Model>(model);
    TraceLog(LOG_INFO, "AssetManager::LoadModel() - Loaded model '%s' from %s (meshes: %d)",
             name.c_str(), filePath.c_str(), model.meshCount);
    return true;
}

bool AssetManager::LoadShader(const std::string& name, const std::string& vsFilePath, const std::string& fsFilePath)
{
    if (m_shaders.find(name) != m_shaders.end())
    {
        TraceLog(LOG_INFO, "AssetManager::LoadShader() - Shader '%s' already cached", name.c_str());
        return true;
    }

    if (!FileExists(vsFilePath) || !FileExists(fsFilePath))
    {
        TraceLog(LOG_ERROR, "AssetManager::LoadShader() - Shader files not found: %s or %s",
                 vsFilePath.c_str(), fsFilePath.c_str());
        return false;
    }

    Shader shader = LoadShader(vsFilePath.c_str(), fsFilePath.c_str());
    if (shader.id == 0)
    {
        TraceLog(LOG_ERROR, "AssetManager::LoadShader() - Failed to load shader: %s + %s",
                 vsFilePath.c_str(), fsFilePath.c_str());
        return false;
    }

    m_shaders[name] = std::make_shared<Shader>(shader);
    TraceLog(LOG_INFO, "AssetManager::LoadShader() - Loaded shader '%s' from %s + %s",
             name.c_str(), vsFilePath.c_str(), fsFilePath.c_str());
    return true;
}

Texture2D* AssetManager::GetTexture(const std::string& name)
{
    auto it = m_textures.find(name);
    return (it != m_textures.end()) ? it->second.get() : nullptr;
}

Font* AssetManager::GetFont(const std::string& name)
{
    auto it = m_fonts.find(name);
    return (it != m_fonts.end()) ? it->second.get() : nullptr;
}

Model* AssetManager::GetModel(const std::string& name)
{
    auto it = m_models.find(name);
    return (it != m_models.end()) ? it->second.get() : nullptr;
}

Shader* AssetManager::GetShader(const std::string& name)
{
    auto it = m_shaders.find(name);
    return (it != m_shaders.end()) ? it->second.get() : nullptr;
}

bool AssetManager::HasTexture(const std::string& name) const
{
    return m_textures.find(name) != m_textures.end();
}

bool AssetManager::HasFont(const std::string& name) const
{
    return m_fonts.find(name) != m_fonts.end();
}

bool AssetManager::HasModel(const std::string& name) const
{
    return m_models.find(name) != m_models.end();
}

bool AssetManager::HasShader(const std::string& name) const
{
    return m_shaders.find(name) != m_shaders.end();
}

void AssetManager::UnloadTexture(const std::string& name)
{
    auto it = m_textures.find(name);
    if (it != m_textures.end())
    {
        UnloadTexture(*it->second);
        m_textures.erase(it);
        TraceLog(LOG_INFO, "AssetManager::UnloadTexture() - Unloaded texture: %s", name.c_str());
    }
}

void AssetManager::UnloadFont(const std::string& name)
{
    auto it = m_fonts.find(name);
    if (it != m_fonts.end())
    {
        UnloadFont(*it->second);
        m_fonts.erase(it);
        TraceLog(LOG_INFO, "AssetManager::UnloadFont() - Unloaded font: %s", name.c_str());
    }
}

void AssetManager::UnloadModel(const std::string& name)
{
    auto it = m_models.find(name);
    if (it != m_models.end())
    {
        UnloadModel(*it->second);
        m_models.erase(it);
        TraceLog(LOG_INFO, "AssetManager::UnloadModel() - Unloaded model: %s", name.c_str());
    }
}

void AssetManager::UnloadShader(const std::string& name)
{
    auto it = m_shaders.find(name);
    if (it != m_shaders.end())
    {
        UnloadShader(*it->second);
        m_shaders.erase(it);
        TraceLog(LOG_INFO, "AssetManager::UnloadShader() - Unloaded shader: %s", name.c_str());
    }
}

void AssetManager::UnloadAll()
{
    size_t textureCount = m_textures.size();
    size_t fontCount = m_fonts.size();
    size_t modelCount = m_models.size();
    size_t shaderCount = m_shaders.size();

    for (auto& pair : m_textures)
    {
        UnloadTexture(*pair.second);
    }
    m_textures.clear();

    for (auto& pair : m_fonts)
    {
        UnloadFont(*pair.second);
    }
    m_fonts.clear();

    for (auto& pair : m_models)
    {
        UnloadModel(*pair.second);
    }
    m_models.clear();

    for (auto& pair : m_shaders)
    {
        UnloadShader(*pair.second);
    }
    m_shaders.clear();

    TraceLog(LOG_INFO, "AssetManager::UnloadAll() - Unloaded all assets: %d textures, %d fonts, %d models, %d shaders",
             textureCount, fontCount, modelCount, shaderCount);
}

size_t AssetManager::GetEstimatedMemoryUsage() const
{
    size_t totalSize = 0;

    // Estimate texture memory (rough calculation)
    for (const auto& pair : m_textures)
    {
        if (pair.second)
        {
            totalSize += pair.second->width * pair.second->height * 4; // Assuming RGBA
        }
    }

    // Estimate model memory (very rough)
    for (const auto& pair : m_models)
    {
        if (pair.second)
        {
            totalSize += pair.second->meshCount * sizeof(Mesh) * 1000; // Rough estimate
        }
    }

    return totalSize;
}

bool AssetManager::FileExists(const std::string& filePath) const
{
    std::ifstream file(filePath);
    return file.good();
}
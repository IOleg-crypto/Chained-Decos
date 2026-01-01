#include "TextureService.h"
#include <iostream>

namespace CHEngine
{
static std::unique_ptr<TextureService> s_Instance = nullptr;

void TextureService::Init()
{
    s_Instance = std::unique_ptr<TextureService>(new TextureService());
}

bool TextureService::IsInitialized()
{
    return s_Instance != nullptr;
}

void TextureService::Shutdown()
{
    if (s_Instance)
    {
        s_Instance->InternalShutdown();
        s_Instance.reset();
    }
}

bool TextureService::LoadTexture(const std::string &name, const std::string &path)
{
    return s_Instance->InternalLoadTexture(name, path);
}

Texture2D TextureService::GetTexture(const std::string &name)
{
    return s_Instance->InternalGetTexture(name);
}

bool TextureService::InternalLoadTexture(const std::string &name, const std::string &path)
{
    if (m_textures.find(name) != m_textures.end())
        return true;

    Texture2D texture = ::LoadTexture(path.c_str());
    if (texture.id == 0)
    {
        std::cerr << "[TextureService] Failed to load texture: " << name << " from " << path
                  << "\n";
        return false;
    }

    m_textures[name] = texture;
    std::cout << "[TextureService] Successfully loaded texture: " << name << "\n";
    return true;
}

Texture2D TextureService::InternalGetTexture(const std::string &name)
{
    auto it = m_textures.find(name);
    if (it != m_textures.end())
        return it->second;

    return {0}; // Return empty texture
}

void TextureService::InternalShutdown()
{
    for (auto &pair : m_textures)
    {
        ::UnloadTexture(pair.second);
    }
    m_textures.clear();
}
} // namespace CHEngine

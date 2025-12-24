#include "TextureService.h"
#include <iostream>

namespace CHEngine
{
bool TextureService::LoadTexture(const std::string &name, const std::string &path)
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

Texture2D TextureService::GetTexture(const std::string &name)
{
    auto it = m_textures.find(name);
    if (it != m_textures.end())
        return it->second;

    return {0}; // Return empty texture
}

void TextureService::Shutdown()
{
    for (auto &pair : m_textures)
    {
        ::UnloadTexture(pair.second);
    }
    m_textures.clear();
}
} // namespace CHEngine

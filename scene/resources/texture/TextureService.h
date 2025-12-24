#ifndef TEXTURE_SERVICE_H
#define TEXTURE_SERVICE_H

#include <raylib.h>
#include <string>
#include <unordered_map>

namespace CHEngine
{
class TextureService
{
public:
    TextureService() = default;
    ~TextureService() = default;

    // Load a texture from file and cache it
    bool LoadTexture(const std::string &name, const std::string &path);

    // Get a cached texture. Returns empty texture if not found.
    Texture2D GetTexture(const std::string &name);

    // Unload all textures
    void Shutdown();

private:
    std::unordered_map<std::string, Texture2D> m_textures;
};
} // namespace CHEngine

#endif // TEXTURE_SERVICE_H

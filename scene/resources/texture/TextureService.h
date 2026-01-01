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
    static void Init();
    static void Shutdown();
    static bool IsInitialized();

    // Load a texture from file and cache it
    static bool LoadTexture(const std::string &name, const std::string &path);

    // Get a cached texture. Returns empty texture if not found.
    static Texture2D GetTexture(const std::string &name);

    ~TextureService() = default;

private:
    TextureService() = default;

    bool InternalLoadTexture(const std::string &name, const std::string &path);
    Texture2D InternalGetTexture(const std::string &name);
    void InternalShutdown();

private:
    std::unordered_map<std::string, Texture2D> m_textures;
};
} // namespace CHEngine

#endif // TEXTURE_SERVICE_H

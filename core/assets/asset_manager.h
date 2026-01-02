#ifndef CD_CORE_ASSETS_ASSET_MANAGER_H
#define CD_CORE_ASSETS_ASSET_MANAGER_H

#include "raylib.h"
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace CHEngine
{

class AssetManager
{
public:
    // Models
    static bool LoadModel(const std::string &name, const std::string &path, bool preload = true);
    static void UnloadAllModels();
    static std::vector<std::string> GetAvailableModels();
    static std::optional<std::reference_wrapper<Model>> GetModel(const std::string &name);

    // Fonts
    static bool LoadFont(const std::string &name, const std::string &path);
    static Font GetFont(const std::string &name);

    // Textures (to be implemented if/when TextureManager exists)
    // static bool LoadTexture(const std::string& name, const std::string& path);
    // static Texture2D GetTexture(const std::string& name);
};

} // namespace CHEngine

#endif // CD_CORE_ASSETS_ASSET_MANAGER_H

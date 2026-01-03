#ifndef CH_ASSET_MANAGER_H
#define CH_ASSET_MANAGER_H

#include <raylib.h>
#include <string>
#include <unordered_map>

namespace CH
{
class AssetManager
{
public:
    static void Init();
    static void Shutdown();

    // Model Management
    static Model LoadModel(const std::string &path);
    static Model GetModel(const std::string &name);
    static bool HasModel(const std::string &name);
    static void UnloadModel(const std::string &name);

    // Texture Management
    static Texture2D LoadTexture(const std::string &path);
    static Texture2D GetTexture(const std::string &name);
    static bool HasTexture(const std::string &name);
    static void UnloadTexture(const std::string &name);

private:
    static std::unordered_map<std::string, Model> s_Models;
    static std::unordered_map<std::string, Texture2D> s_Textures;

    AssetManager() = default;
};
} // namespace CH

#endif // CH_ASSET_MANAGER_H

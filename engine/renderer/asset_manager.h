#ifndef CH_ASSET_MANAGER_H
#define CH_ASSET_MANAGER_H

#include <filesystem>
#include <future>
#include <mutex>
#include <raylib.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace CHEngine
{
struct ModelLoadResult
{
    Model model;
    std::string path;
    bool success;
};

class AssetManager
{
public:
    static void Init();
    static void Shutdown();

    // Model Management (Synchronous)
    static Model LoadModel(std::string_view path);
    static Model GetModel(std::string_view name);
    static bool HasModel(std::string_view name);
    static void UnloadModel(std::string_view name);

    // Animation Management
    static ModelAnimation *LoadAnimation(std::string_view path, int *count);
    static ModelAnimation *GetAnimations(std::string_view path, int *count);

    // Model Management (Asynchronous)
    static std::future<ModelLoadResult> LoadModelAsync(std::string_view path);
    static void LoadModelsAsync(const std::vector<std::string> &paths,
                                std::function<void(float)> progressCallback = nullptr);
    static bool IsModelReady(std::string_view path);

    // Texture Management
    static Texture2D LoadTexture(std::string_view path);
    static Texture2D GetTexture(std::string_view name);
    static bool HasTexture(std::string_view name);
    static void UnloadTexture(std::string_view name);

    // Cubemap Management
    static Texture2D LoadCubemap(std::string_view path);

    // Shader Management
    static Shader LoadShader(std::string_view vsPath, std::string_view fsPath);

    // Helper
    static BoundingBox GetModelBoundingBox(std::string_view path);
    static std::filesystem::path ResolvePath(std::string_view path);
    struct StringHash
    {
        using is_transparent = void;
        size_t operator()(const char *txt) const
        {
            return std::hash<std::string_view>{}(txt);
        }
        size_t operator()(std::string_view txt) const
        {
            return std::hash<std::string_view>{}(txt);
        }
        size_t operator()(const std::string &txt) const
        {
            return std::hash<std::string>{}(txt);
        }
    };

    static std::unordered_map<std::string, Model, StringHash, std::equal_to<>> s_LoadedModels;
    static std::unordered_map<std::string, Texture2D, StringHash, std::equal_to<>> s_LoadedTextures;
    static std::unordered_map<std::string, std::pair<ModelAnimation *, int>, StringHash,
                              std::equal_to<>>
        s_LoadedAnimations;
    static std::mutex s_ModelsCacheMutex;
    static std::mutex s_TexturesCacheMutex;
    static std::mutex s_AnimationsCacheMutex;

    AssetManager() = default;
};
} // namespace CHEngine

#endif // CH_ASSET_MANAGER_H

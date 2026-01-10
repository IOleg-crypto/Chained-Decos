#ifndef CH_ASSET_MANAGER_H
#define CH_ASSET_MANAGER_H

#include "engine/core/thread_pool.h"
#include <future>
#include <mutex>
#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace CH
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
    static Model LoadModel(const std::string &path);
    static Model GetModel(const std::string &name);
    static bool HasModel(const std::string &name);
    static void UnloadModel(const std::string &name);

    // Model Management (Asynchronous)
    static std::future<ModelLoadResult> LoadModelAsync(const std::string &path);
    static void LoadModelsAsync(const std::vector<std::string> &paths,
                                std::function<void(float)> progressCallback = nullptr);
    static bool IsModelReady(const std::string &path);

    // Texture Management
    static Texture2D LoadTexture(const std::string &path);
    static Texture2D GetTexture(const std::string &name);
    static bool HasTexture(const std::string &name);
    static void UnloadTexture(const std::string &name);

    // Cubemap Management
    static Texture2D LoadCubemap(const std::string &path);

    // Helper
    static BoundingBox GetModelBoundingBox(const std::string &path);

private:
    static std::unordered_map<std::string, Model> s_Models;
    static std::unordered_map<std::string, Texture2D> s_Textures;
    static std::mutex s_ModelsMutex;
    static std::mutex s_TexturesMutex;
    static ThreadPool s_ThreadPool;

    AssetManager() = default;
};
} // namespace CH

#endif // CH_ASSET_MANAGER_H

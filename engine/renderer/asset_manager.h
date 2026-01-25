#ifndef CH_ASSET_MANAGER_H
#define CH_ASSET_MANAGER_H

#include "asset.h"
#include "engine/audio/sound_asset.h"
#include "engine/core/task_system.h"
#include "environment.h"
#include "model_asset.h"
#include "shader_asset.h"
#include "texture_asset.h"
#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>

namespace CHEngine
{

class Assets
{
public:
    static void Init();
    static void Shutdown();
    static void Update(); // Process GPU uploads

    // Generic Cache Access
    template <typename T> static std::shared_ptr<T> Get(const std::string &path)
    {
        static_assert(std::is_base_of<Asset, T>::value, "T must inherit from Asset");

        std::lock_guard<std::mutex> lock(s_AssetsMutex);

        if (s_Assets.find(path) != s_Assets.end())
        {
            return std::static_pointer_cast<T>(s_Assets[path]);
        }

        auto asset = T::Load(path);
        if (asset)
        {
            asset->SetState(AssetState::Ready);
            s_Assets[path] = asset;
        }
        return asset;
    }

    template <typename T> static std::shared_ptr<T> GetAsync(const std::string &path)
    {
        static_assert(std::is_base_of<Asset, T>::value, "T must inherit from Asset");

        {
            std::lock_guard<std::mutex> lock(s_AssetsMutex);
            if (s_Assets.find(path) != s_Assets.end())
                return std::static_pointer_cast<T>(s_Assets[path]);

            // Create placeholder
            auto asset = std::make_shared<T>();
            asset->SetPath(path);
            asset->SetState(AssetState::Loading);
            s_Assets[path] = asset;
        }

        // Push loading job to TaskSystem
        TaskSystem::PushTask(
            [path]()
            {
                // T::LoadParse will do the CPU heavy work
                // Then we queue it for GPU upload in Update()
                T::LoadAsync(path);
            });

        return std::static_pointer_cast<T>(s_Assets[path]);
    }

    // Helpers
    static std::shared_ptr<ShaderAsset> LoadShader(const std::string &vsPath,
                                                   const std::string &fsPath);
    static std::shared_ptr<ShaderAsset> LoadShader(const std::string &path);
    static std::shared_ptr<EnvironmentAsset> LoadEnvironment(const std::string &path);
    static std::filesystem::path ResolvePath(const std::string &path);

    // GPU Synchronization
    static void QueueForGPUUpload(std::shared_ptr<Asset> asset);

private:
    struct StringHash
    {
        using is_transparent = void;
        size_t operator()(std::string_view txt) const
        {
            return std::hash<std::string_view>{}(txt);
        }
    };

    static std::unordered_map<std::string, std::shared_ptr<Asset>, StringHash, std::equal_to<>>
        s_Assets;
    static std::mutex s_AssetsMutex;

    static std::vector<std::shared_ptr<Asset>> s_GPUUploadQueue;
    static std::mutex s_GPUQueueMutex;
};

using AssetManager = Assets;

} // namespace CHEngine

#endif // CH_ASSET_MANAGER_H

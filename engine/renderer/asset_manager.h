#ifndef CH_ASSET_MANAGER_H
#define CH_ASSET_MANAGER_H

#include "asset.h"
#include "asset_archive.h"
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

class AssetManager
{
public:
    static void Init();
    static void Shutdown();
    static void Update(); // Process GPU uploads

    // Generic Cache Access
    template <typename T> static std::shared_ptr<T> Get(const std::string &path)
    {
        static_assert(std::is_base_of<Asset, T>::value, "T must inherit from Asset");

        auto cached = AssetArchive::Get(path);
        if (cached)
            return std::static_pointer_cast<T>(cached);

        auto asset = T::Load(path);
        if (asset)
        {
            asset->SetState(AssetState::Ready);
            AssetArchive::Add(path, asset);
        }
        return asset;
    }

    template <typename T> static std::shared_ptr<T> GetAsync(const std::string &path)
    {
        static_assert(std::is_base_of<Asset, T>::value, "T must inherit from Asset");

        auto cached = AssetArchive::Get(path);
        if (cached)
            return std::static_pointer_cast<T>(cached);

        // Create placeholder
        auto asset = std::make_shared<T>();
        asset->SetPath(path);
        asset->SetState(AssetState::Loading);
        AssetArchive::Add(path, asset);

        // Push loading job to TaskSystem
        TaskSystem::PushTask([path]() { T::LoadAsync(path); });

        return asset;
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
    static std::vector<std::shared_ptr<Asset>> s_GPUUploadQueue;
    static std::mutex s_GPUQueueMutex;
};

using Assets = AssetManager;

} // namespace CHEngine

#endif // CH_ASSET_MANAGER_H

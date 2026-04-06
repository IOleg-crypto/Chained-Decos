#ifndef CH_ASSET_MANAGER_H
#define CH_ASSET_MANAGER_H

#include "engine/core/assets/asset_loader.h"
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace CHEngine
{
class AssetManager
{
public:
    AssetManager();
    ~AssetManager();

    static AssetManager& Get();

    // Registry for asset loaders
    void RegisterLoader(AssetType type, std::unique_ptr<IAssetLoader> loader);

    [[nodiscard]] std::string ResolvePath(const std::string& path) const;
    AssetHandle ResolveToHandle(const std::string& path) const;

    // Get by path — loads on first access, returns from cache after that
    template <typename T> std::shared_ptr<T> Get(const std::string& path)
    {
        AssetHandle handle = ResolveToHandle(path);
        if (handle != AssetHandle(0))
        {
            auto asset = GetAsset(handle, T::GetStaticType());
            if (asset)
            {
                return std::static_pointer_cast<T>(asset);
            }
        }
        // Not in cache — load it now
        return Load<T>(path);
    }

    template <typename T> std::shared_ptr<T> Get(AssetHandle handle)
    {
        return std::static_pointer_cast<T>(GetAsset(handle, T::GetStaticType()));
    }

    // Explicit load — creates, loads and caches the asset, returns nullptr on failure
    template <typename T> std::shared_ptr<T> Load(const std::string& path)
    {
        return std::static_pointer_cast<T>(LoadAsset(path, T::GetStaticType()));
    }

    [[nodiscard]] size_t GetPendingFinalizeCount() const;
    [[nodiscard]] size_t GetLoadingAssetCount() const;
    [[nodiscard]] bool HasBackgroundWork() const;

    // Update тепер не знає про типи!
    void Update();

private:
    void ReloadAsset(AssetHandle handle, AssetType type);

public:
    template <typename T> void Reload(const std::string& path)
    {
        AssetHandle handle = ResolveToHandle(path);
        ReloadAsset(handle, T::GetStaticType());
    }

private:
    std::shared_ptr<Asset> GetAsset(AssetHandle handle, AssetType type);
    std::shared_ptr<Asset> LoadAsset(const std::string& path, AssetType type);

    // Тільки одна пласка карта для кешу
    std::unordered_map<AssetHandle, std::shared_ptr<Asset>> m_AssetCache;
    std::unordered_map<AssetType, std::unique_ptr<IAssetLoader>> m_Loaders;

    // Карта для швидкого пошуку handle за шляхом
    mutable std::unordered_map<std::string, AssetHandle> m_PathToHandle;
    mutable std::unordered_map<std::string, std::string> m_PathCache;

    // Async loading support
    std::vector<std::shared_ptr<Asset>> m_PendingAssets;
    mutable std::mutex m_PendingMutex;
    mutable std::recursive_mutex m_AssetLock;
};
} // namespace CHEngine

#endif // CH_ASSET_MANAGER_H

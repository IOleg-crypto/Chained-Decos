#ifndef CH_ASSET_MANAGER_H
#define CH_ASSET_MANAGER_H

#include "engine/core/base.h"
#include "engine/core/assets/asset_loader.h"
#include <filesystem>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace CHEngine
{
struct AssetMetadata
{
    AssetHandle Handle;
    std::string FilePath;
    AssetType Type;
};

class AssetManager
{
public:
    AssetManager();
    ~AssetManager();

    static AssetManager& Get();

    void Initialize(const std::filesystem::path& rootPath = "");
    void Shutdown();

    // Registry for asset loaders
    void RegisterLoader(AssetType type, std::unique_ptr<IAssetLoader> loader);

    void SetRootPath(const std::filesystem::path& path);
    [[nodiscard]] std::filesystem::path GetRootPath() const;
    void AddSearchPath(const std::filesystem::path& path);
    void ClearSearchPaths();

    [[nodiscard]] std::string ResolvePath(const std::string& path) const;

    template <typename T> std::shared_ptr<T> Get(const std::string& path)
    {
        return std::static_pointer_cast<T>(GetAsset(path, T::GetStaticType()));
    }

    template <typename T> std::shared_ptr<T> Get(AssetHandle handle)
    {
        return std::static_pointer_cast<T>(GetAsset(handle, T::GetStaticType()));
    }

    [[nodiscard]] const AssetMetadata& GetMetadata(AssetHandle handle) const;
    [[nodiscard]] int GetPendingCount() const;
    void Update();

    template <typename T> void Reload(const std::string& path)
    {
        ReloadAsset(path, T::GetStaticType());
    }

private:
    // Internal methods to be implemented in .cpp
    std::shared_ptr<Asset> GetAsset(const std::string& path, AssetType type);
    std::shared_ptr<Asset> GetAsset(AssetHandle handle, AssetType type);
    void RemoveAsset(const std::string& path, AssetType type);
    void ReloadAsset(const std::string& path, AssetType type);

private:
    std::filesystem::path m_RootPath;
    std::vector<std::filesystem::path> m_SearchPaths;
    // Registry: AssetType -> Loader
    std::unordered_map<AssetType, std::unique_ptr<IAssetLoader>> m_Loaders;

    // Unified cache: Type -> Path -> Asset
    std::map<AssetType, std::map<std::string, std::shared_ptr<Asset>>> m_AssetCaches;
    std::unordered_map<AssetHandle, AssetMetadata> m_AssetMetadata;
    mutable std::unordered_map<std::string, std::string> m_PathCache;

    // Async loading support
    std::vector<std::shared_ptr<Asset>> m_PendingUploads;
    mutable std::mutex m_PendingUploadsMutex;

    std::vector<std::future<void>> m_Futures;
    mutable std::mutex m_FuturesMutex;
    mutable std::recursive_mutex m_AssetLock;

};
} // namespace CHEngine

#endif // CH_ASSET_MANAGER_H

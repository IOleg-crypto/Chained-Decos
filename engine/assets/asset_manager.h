#ifndef CH_ASSET_MANAGER_H
#define CH_ASSET_MANAGER_H

#include "engine/assets/loaders/asset_loader.h"
#include "engine/core/engine_module.h"
#include "engine/common/timestep.h"
#include <filesystem>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pack
{
class Reader;
}

namespace Chained
{

class AssetManager : public EngineModule
{
public:
    AssetManager();
    ~AssetManager();
    virtual void Initialize();
    virtual void Shutdown();
    void Update(Timestep ts);

    // Registers the loader for a specific asset type.
    void RegisterLoader(AssetType type, AssetLoader loader);

    void SetAssetDirectory(const std::filesystem::path& path)
    {
        m_AssetDirectory = path;
    }
    void SetProjectDirectory(const std::filesystem::path& path)
    {
        m_ProjectDirectory = path;
    }
    void SetEngineRoot(const std::filesystem::path& path)
    {
        m_EngineRoot = path;
    }

    // Pack archive support
    bool OpenPack(const std::filesystem::path& packPath);
    bool IsPacked() const
    {
        return m_PackOpen;
    }
    std::vector<uint8_t> ReadAssetData(const std::string& assetPath);
    // Returns true if the asset exists in the open pack OR on disk at the given path.
    [[nodiscard]] bool HasAsset(const std::string& path) const;
    // Invokes callback once for every item path stored in the open pack archive.
    void EnumeratePackedPaths(const std::function<void(std::string_view)>& callback) const;
    // Reads an asset from the pack by converting an absolute path to a project-relative key.
    // Returns empty vector if the pack is not open, path cannot be relativized, or item not found.
    [[nodiscard]] std::vector<uint8_t> ReadProjectAsset(const std::filesystem::path& absolutePath);

    [[nodiscard]] const std::filesystem::path& GetAssetDirectory() const
    {
        return m_AssetDirectory;
    }
    [[nodiscard]] const std::filesystem::path& GetProjectDirectory() const
    {
        return m_ProjectDirectory;
    }
    [[nodiscard]] const std::filesystem::path& GetEngineRoot() const
    {
        return m_EngineRoot;
    }

    // Resolves a path through the project root and caches the resolved value.
    [[nodiscard]] std::string ResolvePath(const std::string& path) const;
    // Resolves a path to an already-loaded asset handle, or 0 when missing.
    AssetHandle ResolveToHandle(const std::string& path) const;

    // Get by path — returns a cached asset when available, otherwise loads it.
    template <typename T> std::shared_ptr<T> Get(const std::string& path)
    {
        AssetHandle handle = ResolveToHandle(path);
        if (handle != AssetHandle(0))
        {
            auto asset = GetAsset(handle);
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
        return std::static_pointer_cast<T>(GetAsset(handle));
    }

    // Explicit load — creates, loads and caches the asset, returning nullptr on failure.
    template <typename T> std::shared_ptr<T> Load(const std::string& path)
    {
        return std::static_pointer_cast<T>(LoadAsset(path, T::GetStaticType()));
    }

    [[nodiscard]] size_t GetPendingFinalizeCount() const;
    [[nodiscard]] size_t GetLoadingAssetCount() const;
    [[nodiscard]] bool HasBackgroundWork() const;

    // Finalizes completed async loads and calls OnLoaded() within a small per-frame budget.
    // Called every frame from Update(Timestep); exposed for tests and explicit drains (e.g. Shutdown).
    void FinalizePendingLoads();

    // Interval (seconds) between .chasset staleness checks. 0 = disabled.
    void SetHotReloadInterval(float seconds)
    {
        m_HotReloadInterval = seconds;
    }
    float GetHotReloadInterval() const
    {
        return m_HotReloadInterval;
    }

    template <typename T> void Reload(const std::string& path)
    {
        AssetHandle handle = ResolveToHandle(path);
        ReloadAsset(handle, T::GetStaticType());
    }

    // Invalidate cache for a specific path — next LoadAsset call will reload from disk.
    // If deleteFromDisk is true, also removes the .chasset file so re-import is forced.
    void Invalidate(const std::string& path, bool deleteFromDisk = false);

    // Delete the .chasset file for a given model path. Returns true if deleted.
    bool DeleteChasset(const std::string& path);

    // Recursively delete all .chasset files in the asset directory. Returns count deleted.
    size_t DeleteAllChassets();

    // Returns paths of all cached model/texture assets whose source file is newer than load time.
    std::vector<std::string> GetStaleAssets() const;

    // Reload all stale assets detected by GetStaleAssets().
    size_t ReloadAllStale();

    std::shared_ptr<Asset> GetAsset(AssetHandle handle);
    std::shared_ptr<Asset> LoadAsset(const std::string& path, AssetType type);

private:
    struct StaleAsset
    {
        AssetHandle handle;
        AssetType type;
        std::string path;
    };

    void ReloadAsset(AssetHandle handle, AssetType type);
    void CheckAssetHotReload();
    std::vector<StaleAsset> CollectStaleAssets(int thresholdSeconds) const;
    bool ExecuteLoad(const std::shared_ptr<Asset>& asset, AssetLoader* loader, const std::string& resolved);

    // Asset cache. Guarded by m_AssetLock.
    std::unordered_map<AssetHandle, std::shared_ptr<Asset>> m_AssetCache;
    std::unordered_map<AssetType, AssetLoader> m_Loaders;

    // Path-to-handle mapping for quick lookup and a path resolution cache. Guarded by m_AssetLock.
    mutable std::unordered_map<std::string, AssetHandle> m_PathToHandle;
    mutable std::unordered_map<std::string, std::string> m_PathCache;

    // Async loading support. m_PendingAssets guarded by m_PendingMutex.
    std::deque<std::shared_ptr<Asset>> m_PendingAssets;
    mutable std::mutex m_PendingMutex;
    mutable std::recursive_mutex m_AssetLock;

    std::filesystem::path m_AssetDirectory;
    std::filesystem::path m_ProjectDirectory;
    std::filesystem::path m_EngineRoot;

    std::unique_ptr<pack::Reader> m_PackReader;
    bool m_PackOpen = false;

    float m_HotReloadInterval = 3.0f;
    float m_HotReloadAccumulator = 0.0f;
};
} // namespace Chained

#endif // CH_ASSET_MANAGER_H